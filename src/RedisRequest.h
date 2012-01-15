#pragma once

#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/interprocess/detail/atomic.hpp>
#endif
#include "RedisResponse.h"

namespace coconut {

class IOService;
class RedisRequestImpl;

class COCONUT_API RedisRequest {
public:
	RedisRequest(boost::shared_ptr<IOService> ioService, const char *host, int port/*, int timeout*/);
	~RedisRequest();

	class EventHandler {
	public:
		virtual ~EventHandler() { }
		virtual void onRedisRequest_Connected() { }
		virtual void onRedisRequest_Closed() { }
		virtual void onRedisRequest_Response(boost::shared_ptr<RedisResponse> response) { }
		virtual void onRedisRequest_Error(int error, const char *strerror) { }
	};

public:
	void setEventHandler(EventHandler *handler) {
		handler_ = handler;
	}

	EventHandler * eventHandler() {
		return handler_;
	}

	boost::shared_ptr<IOService> ioService();

	void connect();
	void close(bool callback = true);

	// command
	static boost::uint32_t issueTicket() {
		static volatile boost::uint32_t s_ticket = 1;
#if defined(WIN32) 
		boost::interprocess::ipcdetail::atomic_inc32(&s_ticket);
#else
		boost::interprocess::detail::atomic_inc32(&s_ticket);
#endif
		return s_ticket;
	}

	ticket_t get(const std::string &key);
	ticket_t get(int ticket, const std::string &key);

	int set(const std::string &key, const std::string &value);
	int set(int ticket, const std::string &key, const std::string &value);

private:
	RedisRequestImpl *impl_;
	EventHandler *handler_;
};

}
