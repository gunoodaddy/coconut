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
#include "DeferredCaller.h"
#include "IOService.h"
#include "ThreadUtil.h"
#include "InternalLogger.h"
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
		_LOG_TRACE("~DeferredCallerImpl %p %p\n", this, eventDeferred_);
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
