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
#include "Timer.h"
#include "Logger.h"
#include "ThreadUtil.h"
#include <event2/event.h>
#include <map>

namespace coconut {

class TimerImpl {
public:
	TimerImpl(Timer *owner) : owner_(owner) { }
	~TimerImpl() { 
		std::map<int, struct timer_context_t *>::iterator it = mapTimers_.begin();

		for(; it != mapTimers_.end(); it++) {
			struct timer_context_t *context = it->second;
			event_free(context->timer);
			free(context);
		}
	}

private:
	struct timer_context_t {
		int id;
		TimerImpl *self;
		bool repeat;
		struct event *timer;
		struct timeval tv;
	};

	static void timer_cb(coconut_socket_t fd, short what, void *arg) {
		struct timer_context_t *context = (struct timer_context_t *)arg;
		context->self->fire_onTimer_Timer(context);
	}

public:
	void setTimer(int id, unsigned int msec, bool repeat) {
		ScopedMutexLock(timerLock_);
		struct timer_context_t *context = NULL;
		std::map<int, struct timer_context_t *>::iterator it = mapTimers_.find(id);
		if(it != mapTimers_.end()) {
			LOG_DEBUG("OLD TIMER : ioService = %p id = %d sec = %d.%d\n", owner_->ioService().get(), id, msec/1000, msec % 1000);
			context = it->second;
			evtimer_del(context->timer);
		} else {
			LOG_DEBUG("NEW TIMER : ioService = %p id = %d sec = %d.%d\n", owner_->ioService().get(), id, msec/1000, msec % 1000);
			context = (struct timer_context_t *)malloc(sizeof(struct timer_context_t)); 
			context->timer = evtimer_new(owner_->ioService()->coreHandle(), timer_cb, context);

			mapTimers_.insert(std::map<int, struct timer_context_t *>::value_type(id, context));
		}
		assert(context);

		struct timeval tv = MAKE_TIMEVAL_MSEC(msec);
		context->tv = tv;
		context->id = id;
		context->self = this;
		context->repeat = repeat;

		evtimer_add(context->timer, &context->tv);
	}

	void killTimer(int id) {
		ScopedMutexLock(timerLock_);
		std::map<int, struct timer_context_t *>::iterator it = mapTimers_.find(id);

		if(it != mapTimers_.end()) {
			LOG_DEBUG("KILL TIMER : ioService = %p id = %d\n", owner_->ioService().get(), id);
			event_free(it->second->timer);
			free(it->second);
			mapTimers_.erase(it);
		}
	}

	void fire_onTimer_Timer(timer_context_t *context) {
		ScopedMutexLock(timerLock_);

		int id = context->id;
		if(context->repeat) {
			event_add(context->timer, &context->tv);
		} else {
			killTimer(id);
		}

		if(owner_->eventHandler())
			owner_->eventHandler()->onTimer_Timer(id); 
	}

private:
	Timer *owner_;
	std::map<int, struct timer_context_t *> mapTimers_;
	Mutex timerLock_;
};

//---------------------------------------------------------------------------------------
	
Timer::Timer(boost::shared_ptr<IOService> ioService) : ioService_(ioService), handler_(NULL) { 
	impl_ = new TimerImpl(this);
}

Timer::~Timer() {
	delete impl_;
}

void Timer::setTimer(int id, unsigned int msec, bool repeat) {
	impl_->setTimer(id, msec, repeat);
}

void Timer::killTimer(int id) {
	impl_->killTimer(id);
}

}
