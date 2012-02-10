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
#ifndef COCONUT_REDIS_DISABLE
#include "IOService.h"
#include "RedisRequest.h"
#include "DeferredCaller.h"
#include "Exception.h"
#include "ThreadUtil.h"
#include "InternalLogger.h"
#include "Timer.h"
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

#define DEFAULT_RECONNECT_TIMEOUT 	1000	// unit : msec
#define DEFAULT_CONNECT_TIMEOUT 	2000	// unit : msec

namespace coconut {

class RedisRequestImpl : public Timer::EventHandler {
public:
	const static int TIMER_ID_RECONNECT 			= 0x1;
	const static int TIMER_ID_CONNECTION_TIMEOUT 	= 0x3;
	const static int TIMER_ID_COMMAND_TTL 			= 0x5;

	enum RedisRequestState {
		REDISREQUEST_INIT,
		REDISREQUEST_CONNECTING,
		REDISREQUEST_CONNECTED,
		REDISREQUEST_CONNECTFAILED,
		REDISREQUEST_CLOSED,
		REDISREQUEST_RECONECTREADY,
	};

	RedisRequestImpl(RedisRequest *owner, 
					 boost::shared_ptr<IOService> ioService, 
					 const char *host, 
					 int port/*, int timeout*/) 
		: owner_(owner)
		, ioService_(ioService)
		, redisContext_(NULL)
		, host_(host)
		, port_(port)
		, state_(REDISREQUEST_INIT)
		, timerObj_(NULL) 
		, deferredCaller_(ioService) 
	{
		_makeTimer();
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

	void close(bool callback = true) {
		if(redisContext_) {
			if(callback) {
				redisContext_->onConnect = NULL;
				redisContext_->onDisconnect = NULL;
			}

			// TODO gracefully async disconnect logic need
			// CHECK IT OUT!
			redisAsyncFree(redisContext_);
			redisContext_ = NULL;
		}
		state_ = REDISREQUEST_CLOSED;
	}

	void reconnect() {
		_LOG_DEBUG("REDIS RECONNECT START : this = %p", this);
		CHECK_IOSERVICE_STOP_VOID_RETURN(ioService_);
		state_ = REDISREQUEST_RECONECTREADY;
		timerObj_->setTimer(TIMER_ID_RECONNECT, DEFAULT_RECONNECT_TIMEOUT, false);
	}

	void connect() {
		if(redisContext_) {
			throw IllegalStateException("Error redisContext already created");
		}
		state_ = REDISREQUEST_CONNECTING;
		ScopedMutexLock(lockRedis_);
		redisContext_ = redisAsyncConnect(host_.c_str(), port_);

		if (redisContext_->err) {
			_LOG_DEBUG("REDIS CONNECT FAILED (CREATE ERROR): %s:%d, err = %x, this = %p"
				, host_.c_str(), port_, redisContext_->err, this);

			state_ = REDISREQUEST_CONNECTFAILED;
			//fire_onRedisRequest_Error(redisContext_->err, "connect error", NULL);
			// disconnectCallback() is called later soon..
			// error process will be executed by that function.
		}

		redisContext_->data = this;
		redisLibeventAttach(redisContext_, (struct event_base *)ioService_->coreHandle());
		redisAsyncSetConnectCallback(redisContext_, connectCallback);
		redisAsyncSetDisconnectCallback(redisContext_, disconnectCallback);
		timerObj_->setTimer(TIMER_ID_CONNECTION_TIMEOUT, DEFAULT_CONNECT_TIMEOUT, false);

		_LOG_DEBUG("redis connect start : %s:%d, flag = 0x%x, fd = %d, context = %p, this = %p"
				, host_.c_str(), port_, redisContext_->c.flags, redisContext_->c.fd, redisContext_, this);
	}

	ticket_t command(const std::string &cmd, const std::string args, RedisRequest::ResponseHandler handler, int timeout = 0) {

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

		return command(tempArgs, handler, timeout);
	}

	ticket_t command(const std::vector<std::string> &args, RedisRequest::ResponseHandler handler, int timeout = 0) {
		ticket_t ticket = issueTicket();

		if(timeout > 0) {
			timerObj_->setTimer(TIMER_ID_COMMAND_TTL, 1000, true);
		}

		struct RedisRequest::requestContext context;
		context.args = args;
		context.ticket = ticket;
		context.handler = handler;
		context.ttl = timeout > 0 ? timeout : -1;

		// for preventing from multithread race condition, must insert to map here..
		lockRedis_.lock();
		mapCallback_.insert(MapCallback_t::value_type(ticket, context));
		lockRedis_.unlock();

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
		_LOG_DEBUG("redis request canceled : ticket = %d, count = %d/%d <START>"
			, ticket, mapCallback_.size(), reservedCommands_.size());
		_removeContext(ticket);
		_LOG_DEBUG("redis request canceled : ticket = %d, count = %d/%d <END>"
			, ticket, mapCallback_.size(), reservedCommands_.size());
	}

private:
	void _removeContext(ticket_t ticket) {
		ScopedMutexLock(lockRedis_);

		MapCallback_t::iterator it = mapCallback_.find(ticket);
		if(it != mapCallback_.end()) {
			mapCallback_.erase(it);
		}

		VectorReservedContext_t::iterator itR = reservedCommands_.begin();
		for(; itR != reservedCommands_.end(); itR++) {
			if((*itR).ticket == ticket) {
				reservedCommands_.erase(itR);
				break;
			}
		}
	}

	void _makeTimer() {
		if(NULL == timerObj_) {
			timerObj_ = new Timer(ioService());
			timerObj_->setEventHandler(this);
		}
	}
	void _doCommand(ticket_t ticket, int argc, const char **argv, const size_t *argvlen) {
		int ret = redisAsyncCommandArgv(redisContext_, getCallback, (void *)ticket, argc, argv, argvlen);

		if(ret != REDIS_OK) {
			cancel(ticket);
			throw RedisException(ret, "Redis get command failed");
		}
	}

	void _command(ticket_t ticket, const std::vector<std::string> &args) {
		ScopedMutexLock(lockRedis_);

		if(state_ != REDISREQUEST_CONNECTED) {
			struct reservedCommandContext reserved;
			reserved.ticket = ticket;
			reserved.callback = boost::bind(&RedisRequestImpl::_command, this, ticket, args);
			reservedCommands_.push_back(reserved);
			_LOG_DEBUG("redis request reserved : redis is not connected.. ticket = %d, cmd = %s, argsize = %d, this = %p"
				, (int)ticket, args[0].c_str(), args.size(), this);
			return;
		}

		_LOG_DEBUG("redis request start : cmd = %s, argsize = %d, this = %p", args[0].c_str(), args.size(), this);

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
		state_ = REDISREQUEST_CONNECTED;

		timerObj_->killTimer(TIMER_ID_CONNECTION_TIMEOUT);
		_LOG_DEBUG("REDIS CONNECTED : this = %p, fd = %d", this, redisContext_->c.fd);

		for(size_t i = 0; i < reservedCommands_.size(); i++) {
			reservedCommands_[i].callback();	// call
		}
	}

	void fire_onRedisRequest_ConnectFailed() {
		
		ScopedMutexLock(lockRedis_);
		state_ = REDISREQUEST_CONNECTFAILED;
		timerObj_->killTimer(TIMER_ID_CONNECTION_TIMEOUT);

		fire_onRedisRequest_Error(redisContext_->err, redisContext_->errstr, NULL);

		// hiredis lib will free my redisContext_ internally..
		// MUST be set NULL
		redisContext_ = NULL;

		reconnect();
		_LOG_DEBUG("REDIS CONNECT FAILED : this = %p, context = %p", this, redisContext_);
	}

	void fire_onRedisRequest_Closed(int status) {
		// CAUTION!
		// NEVER ACCESS redisContext_ HERE!!!!
		// redisContext_ can be NULL already!

		ScopedMutexLock(lockRedis_);
		_LOG_DEBUG("REDIS CLOSED : %p", this);

		state_ = REDISREQUEST_CLOSED;

		// hiredis lib will free my redisContext_ internally..
		// MUST be set NULL
		redisContext_ = NULL;

		reconnect();
	}

	void fire_onRedisRequest_Error(int error, const char *strerror, void *privdata) {
		if(privdata) {
			ticket_t ticket = (ticket_t)privdata;
			const struct RedisRequest::requestContext *context = _findRequestContext(ticket);
			if(context) {
				boost::shared_ptr<RedisResponse> res(new RedisResponse(error, strerror, ticket));
				context->handler(context, res);
				_removeContext(ticket);
			}
		}
	}

	void fire_onRedisRequest_Response(redisReply *reply, void *privdata) {
		ScopedMutexLock(lockRedis_);

		ticket_t ticket = (ticket_t)privdata;
		boost::shared_ptr<RedisResponse> res(new RedisResponse(reply, ticket));
		const struct RedisRequest::requestContext *context = _findRequestContext(ticket);

		if(context) {
			context->handler(context, res);
			_removeContext(ticket);
		}
	}

	// Timer callback
private:
	void setState(RedisRequestState state) {
		state_ = state;
	}

	void onTimer_Timer(int id) {
		switch(id) {
			case TIMER_ID_CONNECTION_TIMEOUT:
				if(redisContext_) {
					LOG_DEBUG("REDIS CONNECT TIMEOUT : this = %p, context = %p, status = %x/%x, fd = %d\n"
						, this, redisContext_, redisContext_->err, redisContext_->c.flags, redisContext_->c.fd);

					// for preventing <RACE CONDITION> (not thread issue, but hiredis async disconnect logic problem)
					// MUST RESET fd value..
					redisContext_->c.fd = COOKIE_INVALID_SOCKET;
					redisAsyncDisconnect(redisContext_);
					redisContext_ = NULL;
					state_ = REDISREQUEST_CLOSED;
					reconnect();
				} else {
					reconnect();
				}
				break;
			case TIMER_ID_RECONNECT:
				assert(redisContext_ == NULL && "redisContext_ MUST be freed before reconnect()");
				connect();
				break;
			case TIMER_ID_COMMAND_TTL:
			{
				ScopedMutexLock(lockRedis_);
				std::list<ticket_t> cancelList;
				bool needThisTimer = false;
				MapCallback_t::iterator it = mapCallback_.begin();
				for(; it != mapCallback_.end(); it++) {
					if(it->second.ttl > 0) {
						LOG_TRACE("REDIS COMMAND TTL CHECK : this = %p, ticket = %d, ttl = %d\n"
							, this, (int)it->second.ticket, it->second.ttl);
						if(--it->second.ttl <= 0) {
							cancelList.push_back(it->second.ticket);
						}
						needThisTimer = true;
					}
				}

				if(!needThisTimer) {
					timerObj_->killTimer(id);
				}

				if(cancelList.size() > 0) {
					std::list<ticket_t>::iterator itL = cancelList.begin();
					for( ; itL != cancelList.end(); itL++) {
						fire_onRedisRequest_Error(REDIS_ERR_OTHER, "timeout", (void *)(*itL));
						cancel(*itL);
					}
				} 
				break;
			}
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
			SELF->fire_onRedisRequest_ConnectFailed();
			return;
		}
#endif
		SELF->fire_onRedisRequest_Connected();
	}

	static void disconnectCallback(const redisAsyncContext *c, int status) {
		RedisRequestImpl *SELF = (RedisRequestImpl *)c->data;
		SELF->fire_onRedisRequest_Closed(status);
	}

	static void getCallback(redisAsyncContext *c, void *r, void *privdata) {
		RedisRequestImpl *SELF = (RedisRequestImpl *)c->data;
		redisReply *reply = (redisReply *)r;
		if (reply == NULL) {
			SELF->fire_onRedisRequest_Error(c->err, c->errstr, privdata);
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
	RedisRequestState state_;
	typedef std::map<ticket_t, struct RedisRequest::requestContext> MapCallback_t;
	MapCallback_t mapCallback_;
	Timer *timerObj_;
	
	// thread sync call method
	DeferredCaller deferredCaller_;

	struct reservedCommandContext {
		ticket_t ticket;
		DeferredCaller::deferredMethod_t callback;
	};
	typedef std::vector<struct reservedCommandContext> VectorReservedContext_t;
	VectorReservedContext_t reservedCommands_;
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

void RedisRequest::close() {
	impl_->close();
}

void RedisRequest::cancel(ticket_t ticket) {
	impl_->cancel(ticket);
}

ticket_t RedisRequest::command(const std::string &cmd, 
                               const std::string args, 
                               RedisRequest::ResponseHandler handler, 
                               int timeout) 
{
	return impl_->command(cmd, args, handler, timeout);
}

ticket_t RedisRequest::command(const std::vector<std::string> &args, 
                               ResponseHandler handler, 
                               int timeout) 
{
	return impl_->command(args, handler, timeout);
}

}

#endif
