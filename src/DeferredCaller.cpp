#include "Coconut.h"
#include "DeferredCaller.h"
#include "IOService.h"
#include "ThreadUtil.h"
#include "Logger.h"
#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/function.hpp>
#include <boost/bind.hpp>
#endif
#include <event2/event.h>
#include <vector>

namespace coconut {

class DeferredCallerImpl {
public:
	
	DeferredCallerImpl() : eventDeferred_(NULL) { } 

	DeferredCallerImpl(boost::shared_ptr<IOService> ioService) 
		: ioService_(ioService), eventDeferred_(NULL) {
		eventDeferred_ = event_new(ioService->coreHandle(), -1, EV_READ|EV_PERSIST, cb_func, this);
	}

	~DeferredCallerImpl() { 
		LOG_TRACE("~DeferredCallerImpl %p %p\n", this, eventDeferred_);
		if(eventDeferred_)
			event_free(eventDeferred_);
	}

private:
	static void cb_func(coconut_socket_t fd, short what, void *arg) {
		DeferredCallerImpl *SELF = (DeferredCallerImpl *)arg;
		SELF->fireDeferredEvent();
	}

	void fireDeferredEvent() {
		lock_.lock();
		for(size_t i = 0; i < deferredMethods_.size(); i++) {
			deferredMethods_[i]();
		}
		deferredMethods_.clear();
		lock_.unlock();
	}

public:
	void setIOService(boost::shared_ptr<IOService> ioService) {
		ioService_ = ioService;

		if(eventDeferred_) {
			event_free(eventDeferred_);
			eventDeferred_ = NULL;
		}
		eventDeferred_ = event_new(ioService->coreHandle(), -1, EV_READ|EV_PERSIST, cb_func, this);
	}

	void deferredCall(deferedMethod_t func) {
		assert(eventDeferred_ && "event is not installed..");

		lock_.lock();
		deferredMethods_.push_back(func);
		lock_.unlock();

		// this function must be called last in this function for preventing from race-condition.
		event_active(eventDeferred_, EV_READ, 1);
	}

private:
	boost::shared_ptr<IOService> ioService_;
	struct event *eventDeferred_;
	std::vector<deferedMethod_t> deferredMethods_;
	Mutex lock_;
};


DeferredCaller::DeferredCaller() : impl_(NULL) { 
	impl_ = new DeferredCallerImpl;
}

DeferredCaller::DeferredCaller(boost::shared_ptr<IOService> ioService) : impl_(NULL)
{
	impl_ = new DeferredCallerImpl(ioService);
}

DeferredCaller::~DeferredCaller() {
	delete impl_;
}

void DeferredCaller::setIOService(boost::shared_ptr<IOService> ioService) {
	impl_->setIOService(ioService);
}

void DeferredCaller::deferredCall(deferedMethod_t func) {
	impl_->deferredCall(func);
}

}
