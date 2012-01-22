/*
* Copyright (c) 2012, Eunhyuk Kim(gunoodaddy) 
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*   * Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*   * Neither the name of Redis nor the names of its contributors may be used
*     to endorse or promote products derived from this software without
*     specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#include "Coconut.h"
#include "IOService.h"
#include "RedisRequest.h"
#include "DeferredCaller.h"
#include "Exception.h"
#include <event2/event.h>
#include <event2/event_compat.h>
#include <event2/event_struct.h>
#if defined(WIN32)
#include <adapters/libevent.h>
#include <async.h>
#else
#include <hiredis/adapters/libevent.h>
#include <hiredis/async.h>
#endif
#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/function.hpp>
#endif

namespace coconut {

class RedisRequestImpl {
public:
	RedisRequestImpl(RedisRequest *owner, 
					 boost::shared_ptr<IOService> ioService, 
					 const char *host, 
					 int port/*, int timeout*/) 
		: owner_(owner)
		, ioService_(ioService)
		, redisContext_(NULL)
		, host_(host)
		, port_(port)
		, closing_(false)
		, connected_(false)
		, deferredCaller_(ioService) { }

	~RedisRequestImpl() {
		// TODO redis gracefully close test need..
		close(false);
	}

	static boost::uint32_t issueTicket() {
		static volatile boost::uint32_t s_ticket = 1;
#if defined(WIN32) 
		boost::interprocess::ipcdetail::atomic_inc32(&s_ticket);
#else
		boost::interprocess::detail::atomic_inc32(&s_ticket);
#endif
		return s_ticket;
	}

	boost::shared_ptr<IOService> ioService() {
		return ioService_;
	}

	void close(bool callback) {
		if(redisContext_) {
			// TODO gracefully async disconnect logic need
			// CHECK IT OUT!
			closing_ = true;
			if(false == callback) {
				redisContext_->onConnect = NULL;
				redisContext_->onDisconnect = NULL;
			}
			redisAsyncFree(redisContext_);
			redisContext_ = NULL;
		}
	}

	void connect() {
		if(redisContext_) {
			throw IllegalStateException("Error redisContext already created");
		}
		closing_ = false;
		ScopedMutexLock(lockRedis_);
		redisContext_ = redisAsyncConnect(host_.c_str(), port_);
		if (redisContext_->err) {
			// Let *c leak for now...
			throw RedisException(redisContext_->errstr);
		}
		redisContext_->data = this;

		redisLibeventAttach(redisContext_, ioService_->coreHandle());
		redisAsyncSetConnectCallback(redisContext_, connectCallback);
		redisAsyncSetDisconnectCallback(redisContext_, disconnectCallback);
	}

	ticket_t command(const std::string &cmd, const std::vector<std::string> &args, RedisRequest::ResponseHandler handler) {
		ticket_t ticket = issueTicket();

		// for preventing from multithread race condition, must insert to map here..
		mapCallback_.insert(MapCallback_t::value_type(ticket, handler));

		if(ioService_->isCalledInMountedThread() == false) {
			deferredCaller_.deferredCall(boost::bind(&RedisRequestImpl::_command, this, ticket, cmd, args));
			return ticket;
		}

		// must lock here (or deadlock may occur by DeferredCaller's mutex..)
		ScopedMutexLock(lockRedis_);
		_command(ticket, cmd, args);
		return ticket;
	}

	void cancel(ticket_t ticket) {
		ScopedMutexLock(lockRedis_);
		MapCallback_t::iterator it = mapCallback_.find(ticket);
		if(it != mapCallback_.end()) {
			mapCallback_.erase(it);
		}
	}

private:
	void _command(ticket_t ticket, const std::string &cmd, const std::vector<std::string> &args) {
		ScopedMutexLock(lockRedis_);

		if(!connected_) {
			reservedCommands_.push_back(boost::bind(&RedisRequestImpl::_command, this, ticket, cmd, args));
			return;
		}

		int ret = 0;
		if(args.size() == 1) {
			ret = redisAsyncCommand(redisContext_, getCallback, (void *)ticket, "%s %b", cmd.c_str(), 
					args[0].c_str(), args[0].size());
		} else if(args.size() == 2) {
			ret = redisAsyncCommand(redisContext_, getCallback, (void *)ticket, "%s %b %b", cmd.c_str(), 
					args[0].c_str(), args[0].size(),
					args[1].c_str(), args[1].size());
		} else if(args.size() == 3) {
			ret = redisAsyncCommand(redisContext_, getCallback, (void *)ticket, "%s %b %b %b", cmd.c_str(), 
					args[0].c_str(), args[0].size(),
					args[1].c_str(), args[1].size(),
					args[2].c_str(), args[2].size());
		} else if(args.size() == 4) {
			ret = redisAsyncCommand(redisContext_, getCallback, (void *)ticket, "%s %b %b %b %b", cmd.c_str(), 
					args[0].c_str(), args[0].size(),
					args[1].c_str(), args[1].size(),
					args[2].c_str(), args[2].size(),
					args[3].c_str(), args[3].size());
		} else {
			cancel(ticket);
			throw RedisException(ret, "Redis invalid argument count");
		}

		if(ret != REDIS_OK) {
			cancel(ticket);
			throw RedisException(ret, "Redis get command failed");
		}
	}

	RedisRequest::ResponseHandler _findEventHandler(ticket_t ticket) {
		MapCallback_t::iterator it = mapCallback_.find(ticket);
		if(it != mapCallback_.end()) {
			return it->second;
		}
		return NULL;
	}

	void fire_onRedisRequest_Connected() {
		ScopedMutexLock(lockRedis_);
		connected_ = true;

		for(size_t i = 0; i < reservedCommands_.size(); i++) {
			reservedCommands_[i]();	// call
		}
	}

	void fire_onRedisRequest_Closed() {
		ScopedMutexLock(lockRedis_);
		connected_ = false;
	}

	void fire_onRedisRequest_Error(int error, const char *strerror) {
		if(false == closing_) {
			// TODO redis connect error callback
		}
	}

	void fire_onRedisRequest_Response(redisReply *reply, void *privdata) {
		ScopedMutexLock(lockRedis_);

		//redisAsyncDisconnect(c); // TODO when disconnect??
		ticket_t ticket = (ticket_t)privdata;
		std::string str;
		str.assign(reply->str, reply->len);
		boost::shared_ptr<RedisResponse> res(new RedisResponse(reply, ticket));
		RedisRequest::ResponseHandler handler = _findEventHandler(ticket);

		if(handler) {
			handler(res);
		}
	}

	// redis callback
private:
#if defined(WIN32) 
	static void connectCallback(const redisAsyncContext *c) {
#else
	static void connectCallback(const redisAsyncContext *c, int status) {
#endif
		RedisRequestImpl *SELF = (RedisRequestImpl *)c->data;
#if ! defined(WIN32) 
		if (status != REDIS_OK) {
			SELF->fire_onRedisRequest_Error(c->err, c->errstr);
			return;
		}
#endif
		SELF->fire_onRedisRequest_Connected();
	}

	static void disconnectCallback(const redisAsyncContext *c, int status) {
		RedisRequestImpl *SELF = (RedisRequestImpl *)c->data;
		SELF->fire_onRedisRequest_Closed();
	}

	static void getCallback(redisAsyncContext *c, void *r, void *privdata) {
		RedisRequestImpl *SELF = (RedisRequestImpl *)c->data;
		redisReply *reply = (redisReply *)r;
		if (reply == NULL) {
			SELF->fire_onRedisRequest_Error(c->err, c->errstr);
			return;
		}
		SELF->fire_onRedisRequest_Response(reply, privdata);
	}

private:
	RedisRequest *owner_;
	boost::shared_ptr<IOService> ioService_;	
	redisAsyncContext *redisContext_;
	std::string host_;
	int port_;
	Mutex lockRedis_;
	volatile bool closing_;
	volatile bool connected_;
	typedef std::map<ticket_t, RedisRequest::ResponseHandler> MapCallback_t;
	MapCallback_t mapCallback_;
	
	// thread sync call method
	DeferredCaller deferredCaller_;
	std::vector<deferedMethod_t> reservedCommands_;
};

//---------------------------------------------------------------------------------------------------------------

RedisRequest::RedisRequest(boost::shared_ptr<IOService> ioService, const char *host, int port/*, int timeout*/)  {
	impl_ = new RedisRequestImpl(this, ioService, host, port);
}

RedisRequest::~RedisRequest() {
	delete impl_;
}

boost::shared_ptr<IOService> RedisRequest::ioService() {
	return impl_->ioService();
}

void RedisRequest::connect() {
	impl_->connect();
}

void RedisRequest::close(bool callback) {
	impl_->close(callback);
}

void RedisRequest::cancel(ticket_t ticket) {
	impl_->cancel(ticket);
}

ticket_t RedisRequest::command(const std::string &cmd, const std::vector<std::string> &args, ResponseHandler handler) {
	return impl_->command(cmd, args, handler);
}

}
