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

#include "CoconutLib.h"
#ifndef COCONT_REDIS_DISABLE
#include "IOService.h"
#include "RedisRequest.h"
#include "DeferredCaller.h"
#include "Exception.h"
#include "ThreadUtil.h"
#include "InternalLogger.h"
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
		, deferredCaller_(ioService) { 
		_LOG_TRACE("RedisRequestImpl() : %p", this);
	}
	

	~RedisRequestImpl() {
		// TODO redis gracefully close test need..
		close(false);
		_LOG_TRACE("~RedisRequestImpl() : %p", this);
	}

	static boost::uint32_t issueTicket() {
		static volatile boost::uint32_t s_ticket = 1;
		return atomicIncreaseInt32(&s_ticket);
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
		_LOG_DEBUG("redis connect start : %s:%d", host_.c_str(), port_);
	}

	ticket_t _commandPrepare(const std::vector<std::string> &args, RedisRequest::ResponseHandler handler) {
		ticket_t ticket = issueTicket();

		// for preventing from multithread race condition, must insert to map here..
		struct RedisRequest::requestContext context;
		context.args = args;
		context.ticket = ticket;
		context.handler = handler;

		lockRedis_.lock();
		mapCallback_.insert(MapCallback_t::value_type(ticket, context));
		lockRedis_.unlock();
		return ticket;
	}

	ticket_t command(const std::string &cmd, const std::string args, RedisRequest::ResponseHandler handler) {

		int cnt = 0;
		size_t pos = 0;
		size_t pos2 = 0;
		std::string str;
		std::vector<std::string> tempArgs;

		tempArgs.push_back(cmd);

		do {
			pos2 = args.find(" ", pos);
			str = args.substr(pos, (pos2 - pos));

#define TRIM_SPACE " \t\r\n\v"
			str.erase(str.find_last_not_of(TRIM_SPACE)+1);
			str.erase(0,str.find_first_not_of(TRIM_SPACE));

			if(!str.empty()) {
				tempArgs.push_back(str);
			}
			pos = args.find_first_not_of(" ", pos2);
			if(pos == std::string::npos)
				break;
			cnt++;
		} while(true);

		return command(tempArgs, handler);
	}

	ticket_t command(const std::vector<std::string> &args, RedisRequest::ResponseHandler handler) {
		ticket_t ticket = _commandPrepare(args, handler);

		if(ioService_->isCalledInMountedThread() == false) {
			deferredCaller_.deferredCall(boost::bind(&RedisRequestImpl::_command, this, ticket, args));
			return ticket;
		}

		// must lock here (or deadlock may occur by DeferredCaller's mutex..)
		lockRedis_.lock();
		_command(ticket, args);
		lockRedis_.unlock();
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
	void _doCommand(ticket_t ticket, int argc, const char **argv, const size_t *argvlen) {
		int ret = redisAsyncCommandArgv(redisContext_, getCallback, (void *)ticket, argc, argv, argvlen);

		if(ret != REDIS_OK) {
			cancel(ticket);
			throw RedisException(ret, "Redis get command failed");
		}
	}

	void _command(ticket_t ticket, const std::vector<std::string> &args) {
		ScopedMutexLock(lockRedis_);

		if(!connected_) {
			reservedCommands_.push_back(boost::bind(&RedisRequestImpl::_command, this, ticket, args));
			return;
		}

		_LOG_DEBUG("redis request start : cmd = %s, argsize = %d", args[0].c_str(), args.size());

		std::vector<const char *> tempArgs;	
		std::vector<size_t> tempArgLens;

		for(size_t i = 0; i < args.size(); i++) {
			tempArgs.push_back(args[i].c_str());
			tempArgLens.push_back(args[i].size());
		}

		_doCommand(ticket, args.size(), (const char **)&tempArgs[0], (const size_t *)&tempArgLens[0]);
	}

	const struct RedisRequest::requestContext * _findRequestContext(ticket_t ticket) {
		MapCallback_t::iterator it = mapCallback_.find(ticket);
		if(it != mapCallback_.end()) {
			return &it->second;
		}
		return NULL;
	}

	void fire_onRedisRequest_Connected() {
		ScopedMutexLock(lockRedis_);
		connected_ = true;

		_LOG_DEBUG("redis connected..\n");

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
		const struct RedisRequest::requestContext *context = _findRequestContext(ticket);

		if(context) {
			context->handler(context, res);
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
	typedef std::map<ticket_t, struct RedisRequest::requestContext> MapCallback_t;
	MapCallback_t mapCallback_;
	
	// thread sync call method
	DeferredCaller deferredCaller_;
	std::vector<DeferredCaller::deferedMethod_t> reservedCommands_;
};

//---------------------------------------------------------------------------------------------------------------

RedisRequest::RedisRequest(boost::shared_ptr<IOService> ioService, const char *host, int port/*, int timeout*/)  {
	impl_ = new RedisRequestImpl(this, ioService, host, port);
	_LOG_TRACE("RedisRequest() : %p", this);
}

RedisRequest::~RedisRequest() {
	_LOG_TRACE("~RedisRequest() : %p", this);
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

ticket_t RedisRequest::command(const std::string &cmd, const std::string args, RedisRequest::ResponseHandler handler) {
	return impl_->command(cmd, args, handler);
}

ticket_t RedisRequest::command(const std::vector<std::string> &args, ResponseHandler handler) {
	return impl_->command(args, handler);
}

}

#endif
