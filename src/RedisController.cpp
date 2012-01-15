#include "Coconut.h"
#include "IOService.h"
#include "RedisController.h"
#include "RedisRequest.h"

namespace coconut {

RedisController::~RedisController() {
	LOG_TRACE("~RedisController() : %p\n", this);
}


boost::shared_ptr<IOService> RedisController::ioService() {
	return request_->ioService();
}


BaseIOServiceContainer *RedisController::ioServiceContainer() {
	return request_->ioService()->ioServiceContainer();
}


ticket_t RedisController::set(const std::string &key, const std::string &value, BaseController *observer) {
	ScopedIOServiceLock(ioService());

	int ticket = RedisRequest::issueTicket();
	eventGotResponse()->registerObserver(ticket, observer);
	if(redisRequest()->set(ticket, key, value) <= 0) {
		assert(false && "RedisRequest failed");
		eventGotResponse()->unregisterObserver(ticket, observer);
		return 0;	// failed
	}
	return ticket;
}


ticket_t RedisController::get(const std::string &key, BaseController *observer) {
	ScopedIOServiceLock(ioService());

	int ticket = RedisRequest::issueTicket();
	eventGotResponse()->registerObserver(ticket, observer);
	if(redisRequest()->get(ticket, key) <= 0) {
		assert(false && "RedisRequest failed");
		eventGotResponse()->unregisterObserver(ticket, observer);
		return 0;	// failed
	}
	return ticket;
}

boost::shared_ptr<RedisResponse> RedisController::getAndDeleteResponseOfTicket(ticket_t ticket, bool autoUnregister) {
	boost::shared_ptr<RedisResponse> response;	
	// block
	{
		ScopedMutexLock(lockResponse_);
		mapResponse_t::iterator it = mapResponse_.find(ticket);
		if(it == mapResponse_.end())
			throw NoSuchElementException();

		response = it->second;
		mapResponse_.erase(it);
	}

	if(autoUnregister) {
		eventGotResponse()->clearObserver(ticket); 
	}
	return response;
}

void RedisController::onRedisRequest_Connected() { 
	onConnected();
}
void RedisController::onRedisRequest_Closed() { 
	onClosed();
	eventClosedConnection()->fireObservers(shared_from_this(), 0);
}
void RedisController::onRedisRequest_Response(boost::shared_ptr<RedisResponse> response) { 
	onResponse(response);

	// for response object management..
	lockResponse_.lock();
	mapResponse_.insert(mapResponse_t::value_type(response->ticket(), response));
	lockResponse_.unlock();

	eventGotResponse()->fireObservers(shared_from_this(), response->ticket());
}

void RedisController::onRedisRequest_Error(int error, const char *strerror) { 
	onError(error, strerror);
	eventOccuredError()->fireObservers(shared_from_this(), error);
}

void RedisController::onTimer_Timer(int id) {
	if(id == TIMERID_RECONNECT) {
		timerObj_->killTimer(id);

		// TODO : retry max check..
		retryConnectCnt_++;

		// TODO redis reconnect!
		redisRequest()->close();
		redisRequest()->connect();
	}
	
	BaseController::onTimer_Timer(id);
}

} // end of namespace coconut
