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
		, deferredCaller_(ioService) { }

	~RedisRequestImpl() {
		// TODO redis gracefully close test need..
		close(false);
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

	ticket_t get(ticket_t ticket, const std::string &key) {
		if(ioService_->isCalledInMountedThread() == false) {
			deferredCaller_.deferredCall(boost::bind(&RedisRequestImpl::get, this, ticket, key));
			return ticket;
		}
		ScopedMutexLock(lockRedis_);
		int ret = redisAsyncCommand(redisContext_, getCallback, (void *)ticket, "GET %s", key.c_str());
		if(ret != REDIS_OK) {
			throw RedisException(ret, "Redis get command failed");
		}
		return ticket;
	}

	ticket_t get(const std::string &key) {
		ticket_t ticket = RedisRequest::issueTicket();
		if(ioService_->isCalledInMountedThread() == false) {
			deferredCaller_.deferredCall(boost::bind(&RedisRequestImpl::get, this, ticket, key));
			return ticket;
		}
		return get(ticket, key);
	}

	ticket_t set(ticket_t ticket, const std::string &key, const std::string &value) {
		if(ioService_->isCalledInMountedThread() == false) {
			deferredCaller_.deferredCall(boost::bind(&RedisRequestImpl::set, this, ticket, key, value));
			return ticket;
		}
		ScopedMutexLock(lockRedis_);
		int ret = redisAsyncCommand(redisContext_, getCallback, (void *)ticket, "SET %s %b", key.c_str(), value.c_str(), value.size());
		if(ret != REDIS_OK) {
			throw RedisException(ret, "Redis set command failed");
		}
		return ticket;
	}

	ticket_t set(const std::string &key, const std::string &value) {
		ticket_t ticket = RedisRequest::issueTicket();
		if(ioService_->isCalledInMountedThread() == false) {
			deferredCaller_.deferredCall(boost::bind(&RedisRequestImpl::set, this, ticket, key, value));
			return ticket;
		}

		return set(ticket, key, value);
	}

	void fire_onRedisRequest_Connected() {
		owner_->eventHandler()->onRedisRequest_Connected();
	}

	void fire_onRedisRequest_Error(int error, const char *strerror) {
		if(false == closing_)
			owner_->eventHandler()->onRedisRequest_Error(error, strerror);
	}

	void fire_onRedisRequest_Closed() {
		owner_->eventHandler()->onRedisRequest_Closed();
	}

	void fire_onRedisRequest_Response(redisReply *reply, void *privdata) {
		//redisAsyncDisconnect(c); // TODO when disconnect??
		int ticket = (int)privdata;
		std::string str;
		str.assign(reply->str, reply->len);
		boost::shared_ptr<RedisResponse> res(new RedisResponse(reply, ticket));
		owner_->eventHandler()->onRedisRequest_Response(res);
	}

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
	//Mutex lockRedis_;
	bool closing_;
	
	// thread sync call method
	DeferredCaller deferredCaller_;
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

ticket_t RedisRequest::get(const std::string &key) {
	return impl_->get(key);
}

ticket_t RedisRequest::get(int ticket, const std::string &key) {
	return impl_->get(ticket, key);
}

int RedisRequest::set(const std::string &key, const std::string &value) {
	return impl_->set(key, value);
}

int RedisRequest::set(int ticket, const std::string &key, const std::string &value) {
	return impl_->set(ticket, key, value);
}

}
