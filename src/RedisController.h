#pragma once

#include "RedisRequest.h"
#include "BaseController.h"

#define REDIS_CTRL(baseController) boost::static_pointer_cast<RedisController>(baseController)

namespace coconut {

class COCONUT_API RedisController : public BaseController
                      , public RedisRequest::EventHandler
{
public:
	static const int TIMERID_RECONNECT = (1|INTERNAL_TIMER_BIT);

	RedisController() : retryConnectCnt_(0) {
	}
	virtual ~RedisController();

	// strategy pattern
	void setRedisRequest(boost::shared_ptr<RedisRequest> request) {
		request_ = request;

		_onPreInitialized();
	}

	boost::shared_ptr<RedisRequest> redisRequest() {
		return request_;
	}

	boost::shared_ptr<IOService> ioService();
	BaseIOServiceContainer *ioServiceContainer();

	boost::shared_ptr<RedisResponse> getAndDeleteResponseOfTicket(ticket_t ticket, bool autoUnregister = true);

	ticket_t set(const std::string &key, const std::string &value, BaseController *observer);
	ticket_t get(const std::string &key, BaseController *observer);

private:
	void onRedisRequest_Connected();
	void onRedisRequest_Closed();
	void onRedisRequest_Response(boost::shared_ptr<RedisResponse> response);
	void onRedisRequest_Error(int error, const char *strerror);
	void onTimer_Timer(int id);

protected:
	// RedisController callback event
	virtual void onConnected() { }
	virtual void onClosed() { }
	virtual void onError(int error, const char *strerror) { }
	virtual void onResponse(boost::shared_ptr<RedisResponse> response) { }

private:
	boost::shared_ptr<RedisRequest> request_;

	typedef std::map< ticket_t, boost::shared_ptr<RedisResponse> > mapResponse_t;
	mapResponse_t mapResponse_;
	Mutex lockResponse_;
	int retryConnectCnt_;
};

} // end of namespace coconut
