#include "Coconut.h"
#include "BaseController.h"
#include "IOService.h"

namespace coconut {

void BaseController::setTimer(unsigned short id, unsigned int msec, bool repeat) {
	ScopedIOServiceLock(ioService());
	_makeTimer();
	timerObj_->setTimer(id, msec, repeat);
}

void BaseController::killTimer(short id) {
	ScopedIOServiceLock(ioService());
	_makeTimer();
	timerObj_->killTimer(id);
}

void BaseController::_makeTimer() {
	if(NULL == timerObj_) {
		timerObj_ = new Timer(ioService());
		timerObj_->setEventHandler(this);
	}
}

void BaseController::onTimer_Timer(int id) {
	if(!(id & INTERNAL_TIMER_BIT)) {
		onTimer(id);
	}
}


} // end of namespace coconut
