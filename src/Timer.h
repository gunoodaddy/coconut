#pragma once

#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#endif

#if defined(_MSC_VER)
#pragma warning ( disable: 4231 4251 4275 4786 )
#endif

#define INTERNAL_TIMER_BIT	0x40000000

namespace coconut {

class IOService;
class TimerImpl;

class COCONUT_API Timer {
public:
	Timer(boost::shared_ptr<IOService> ioService);
	~Timer();

	class EventHandler {
	public:
		virtual ~EventHandler() { }
		virtual void onTimer_Timer(int id) { }
	};

public:
	void setEventHandler(EventHandler *handler) {
		handler_ = handler;
	}

	EventHandler* eventHandler() {
		return handler_;
	}

	boost::shared_ptr<IOService> ioService() {
		return ioService_;
	}

	void setTimer(int id, unsigned int msec, bool repeat = true);
	void killTimer(int id);

private:
	TimerImpl *impl_;
	boost::shared_ptr<IOService> ioService_;	
	EventHandler *handler_;
};

}

