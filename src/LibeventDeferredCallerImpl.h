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

#pragma once

#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/function.hpp>
#include <boost/bind.hpp>
#endif
#include <event2/event.h>
#include <vector>
#include "DeferredCallerImpl.h"

namespace coconut {

class LibeventDeferredCallerImpl : public DeferredCallerImpl {
public:
	LibeventDeferredCallerImpl() : DeferredCallerImpl() { }

	LibeventDeferredCallerImpl(boost::shared_ptr<IOService> ioService) 
		: DeferredCallerImpl(ioService), eventDeferred_(NULL) {

		createHandle();
	}

	~LibeventDeferredCallerImpl() { 
		_LOG_TRACE("~LibeventDeferredCallerImpl %p %p\n", this, eventDeferred_);
		destroyHandle();
	}

	void createHandle() {
		eventDeferred_ = event_new((struct event_base *)ioService_->coreHandle(), -1, EV_READ|EV_PERSIST, cb_func, this);
	}

	void destroyHandle() {
		if(eventDeferred_) {
			event_free(eventDeferred_);
			eventDeferred_ = NULL;
		}
	}

	void triggerDeferredEvent() {
		event_active(eventDeferred_, EV_READ, 1);
	}

private:
	static void cb_func(coconut_socket_t fd, short what, void *arg) {
		LibeventDeferredCallerImpl *SELF = (LibeventDeferredCallerImpl *)arg;
		SELF->fireDeferredEvent();
	}

private:
	struct event *eventDeferred_;
};

}
