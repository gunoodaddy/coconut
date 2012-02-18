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
	BaseController() : timerObj_(NULL), deferredCaller_(NULL) {
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
			Timer::destroy(timerObj_);
		if(deferredCaller_)
			DeferredCaller::destroy(deferredCaller_);
	}

public:
	virtual BaseIOServiceContainer *ioServiceContainer()  = 0;
	virtual boost::shared_ptr<IOService> ioService() = 0;

public:
	void setTimer(unsigned short id, unsigned int msec, bool repeat);
	void killTimer(unsigned short id);

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
		deferredCaller_ = DeferredCaller::make();
		deferredCaller_->setIOService(ioService());

		controllerEvent_ClosedConnection_->setDeferredCaller(deferredCaller_);	
		controllerEvent_OccuredError_->setDeferredCaller(deferredCaller_);	
		controllerEvent_GotProtocol_->setDeferredCaller(deferredCaller_);	
		controllerEvent_GotResponse_->setDeferredCaller(deferredCaller_);	

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
	DeferredCaller *deferredCaller_;
};

} // end of namespace coconut
