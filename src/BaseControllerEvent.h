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

#include <set>
#include <vector>
#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#endif
#include "ThreadUtil.h"
#include "DeferredCaller.h"
#include "BaseProtocol.h"

namespace coconut {

class BaseController;

typedef std::set<BaseController*> setObservers_t;

class BaseControllerEvent {
public:
	BaseControllerEvent() : deferredCaller_(NULL) { }

	virtual ~BaseControllerEvent() { }

	DeferredCaller * deferredCaller() {
		return deferredCaller_;
	}

	void setDeferredCaller(DeferredCaller *deferredCaller) {
		deferredCaller_ = deferredCaller;
	}

protected:
	DeferredCaller *deferredCaller_;
	Mutex lockObserver_;
};



class GotProtocolControllerEvent : public BaseControllerEvent {
public:
	class EventHandler {
	public:
		virtual ~EventHandler() { }
		virtual void _onPreControllerEvent_GotProtocol(
						boost::shared_ptr<BaseController> controller, 
						boost::shared_ptr<protocol::BaseProtocol> prot) { }

		virtual void onControllerEvent_GotProtocol(
						boost::shared_ptr<BaseController> controller, 
						boost::shared_ptr<protocol::BaseProtocol> prot) { }
	};

	void registerObserver(BaseController *observer) {
		lockObserver_.lock();
		observers_.insert(observer);
		lockObserver_.unlock();
	}

	void unregisterObserver(BaseController *observer) {
		lockObserver_.lock();
		observers_.erase(observer);
		lockObserver_.unlock();
	}

	void fireObservers(
			boost::shared_ptr<BaseController> targetController, 
			boost::shared_ptr<protocol::BaseProtocol> prot);

private:
	setObservers_t observers_;
};


class ClosedConnectionControllerEvent : public BaseControllerEvent {
public:
	class EventHandler {
	public:
		virtual ~EventHandler() { }
		virtual void _onPreControllerEvent_ClosedConnection(
						boost::shared_ptr<BaseController> controller, 
						int error) { }

		virtual void onControllerEvent_ClosedConnection(
						boost::shared_ptr<BaseController> controller, 
						int error) { }
	};

	void registerObserver(BaseController *observer) {
		lockObserver_.lock();
		observers_.insert(observer);
		lockObserver_.unlock();
	}

	void unregisterObserver(BaseController *observer) {
		lockObserver_.lock();
		observers_.erase(observer);
		lockObserver_.unlock();
	}

	void fireObservers(
			boost::shared_ptr<BaseController> targetController, 
			int error);

private:
	setObservers_t observers_;
};


class OccuredErrorControllerEvent : public BaseControllerEvent {
public:
	class EventHandler {
	public:
		virtual ~EventHandler() { }
		virtual void _onPreControllerEvent_OccuredError(
						boost::shared_ptr<BaseController> controller, 
						int error) { }

		virtual void onControllerEvent_OccuredError(
						boost::shared_ptr<BaseController> controller, 
						int error) { }
	};

	void registerObserver(BaseController *observer) {
		lockObserver_.lock();
		observers_.insert(observer);
		lockObserver_.unlock();
	}

	void unregisterObserver(BaseController *observer) {
		lockObserver_.lock();
		observers_.erase(observer);
		lockObserver_.unlock();
	}

	void fireObservers(
			boost::shared_ptr<BaseController> targetController, 
			int error);

private:
	setObservers_t observers_;
};


class GotResponseControllerEvent : public BaseControllerEvent {
public:
	class EventHandler {
	public:
		virtual ~EventHandler() { }
		virtual void _onPreControllerEvent_GotResponse(
						boost::shared_ptr<BaseController> controller, 
						int ticket) { }

		virtual void onControllerEvent_GotResponse(
						boost::shared_ptr<BaseController> controller, 
						int ticket) { }
	};

	void registerObserver(int ticket, BaseController *observer) {
		lockObserver_.lock();
		
		mapTicketObservers_t::iterator it = observers_.find(ticket);
		if(it != observers_.end()) {
			it->second.insert(observer);
		} else {
			setObservers_t setTemp;
			setTemp.insert(observer);
			observers_.insert(mapTicketObservers_t::value_type(ticket, setTemp));
		}
		lockObserver_.unlock();
	}

	void unregisterObserver(int ticket, BaseController *observer) {
		lockObserver_.lock();
		mapTicketObservers_t::iterator it = observers_.find(ticket);
		if(it != observers_.end()) {
			it->second.erase(observer);

			if(it->second.size() <= 0) {
				observers_.erase(it);
			}
		}
		lockObserver_.unlock();
	}

	void clearObserver(int ticket) {
		lockObserver_.lock();
		mapTicketObservers_t::iterator it = observers_.find(ticket);
		if(it != observers_.end()) {
			// all clear!
			observers_.erase(it);
		}
		lockObserver_.unlock();
	}

	void fireObservers(
			boost::shared_ptr<BaseController> targetController, 
			int ticket);

private:
	typedef std::map<int, setObservers_t> mapTicketObservers_t;
	mapTicketObservers_t observers_;
};

}

