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
			context->timer = evtimer_new(owner_->ioService()->base(), timer_cb, context);

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
