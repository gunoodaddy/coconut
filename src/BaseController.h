#pragma once
#include "config.h"
#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif
#include "BaseIOServiceContainer.h"
#include "Timer.h"
#include "BaseControllerEvent.h"

namespace coconut {

class IOService;

class COCONUT_API BaseController : public boost::enable_shared_from_this<BaseController> 
                     , public Timer::EventHandler
                     , public GotResponseControllerEvent::EventHandler
                     , public GotProtocolControllerEvent::EventHandler
                     , public ClosedConnectionControllerEvent::EventHandler
                     , public OccuredErrorControllerEvent::EventHandler 
{
public:
	BaseController() : timerObj_(NULL) {
		controllerEvent_GotResponse_ = new GotResponseControllerEvent;
		controllerEvent_GotProtocol_ = new GotProtocolControllerEvent;
		controllerEvent_OccuredError_ = new OccuredErrorControllerEvent;
		controllerEvent_ClosedConnection_ = new ClosedConnectionControllerEvent;
	}

	virtual ~BaseController() {
		delete controllerEvent_GotResponse_;
		delete controllerEvent_GotProtocol_;
		delete controllerEvent_OccuredError_;
		delete controllerEvent_ClosedConnection_;
		if(timerObj_)
			delete timerObj_;
	}

public:
	virtual BaseIOServiceContainer *ioServiceContainer()  = 0;
	virtual boost::shared_ptr<IOService> ioService() = 0;

public:
	void setTimer(unsigned short id, unsigned int msec, bool repeat);
	void killTimer(short id);

	GotResponseControllerEvent *eventGotResponse() {
		return controllerEvent_GotResponse_;
	}
	GotProtocolControllerEvent *eventGotProtocol() {
		return controllerEvent_GotProtocol_;
	}
	ClosedConnectionControllerEvent *eventClosedConnection() {
		return controllerEvent_ClosedConnection_;
	}
	OccuredErrorControllerEvent *eventOccuredError() {
		return controllerEvent_OccuredError_;
	}

protected:
	void _makeTimer();

	virtual void _onPreInitialized() {
		controllerEvent_GotResponse_->deferredCaller().setIOService(ioService());
		controllerEvent_GotProtocol_->deferredCaller().setIOService(ioService());
		controllerEvent_ClosedConnection_->deferredCaller().setIOService(ioService());
		controllerEvent_OccuredError_->deferredCaller().setIOService(ioService());

		onInitialized();
	}

	void _onPreControllerEvent_GotResponse(
					boost::shared_ptr<BaseController> controller, 
					int ticket) { 
		onControllerEvent_GotResponse(controller, ticket);
	}

	void _onPreControllerEvent_GotProtocol(
					boost::shared_ptr<BaseController> controller, 
					boost::shared_ptr<protocol::BaseProtocol> prot) {
		onControllerEvent_GotProtocol(controller, prot);
	}

	void _onPreControllerEvent_ClosedConnection(
					boost::shared_ptr<BaseController> controller, 
					int error) {
		onControllerEvent_ClosedConnection(controller, error);
	}

	void _onPreControllerEvent_OccuredError(
					boost::shared_ptr<BaseController> controller, 
					int error) {
		onControllerEvent_OccuredError(controller, error);
	}

	virtual void onTimer_Timer(int id);

protected:
	// BaseController callback event
	virtual void onInitialized() { }
	virtual void onTimer(unsigned short id) { }

protected:
	friend class GotResponseControllerEvent;
	friend class GotProtocolControllerEvent;
	friend class ClosedConnectionControllerEvent;
	friend class OccuredErrorControllerEvent;

	GotResponseControllerEvent *controllerEvent_GotResponse_;
	GotProtocolControllerEvent *controllerEvent_GotProtocol_;
	ClosedConnectionControllerEvent *controllerEvent_ClosedConnection_;
	OccuredErrorControllerEvent *controllerEvent_OccuredError_;
	
	Timer *timerObj_;
};

} // end of namespace coconut
