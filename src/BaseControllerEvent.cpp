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

#include "CoconutLib.h"
#include "BaseControllerEvent.h"
#include "BaseController.h"
#include "IOService.h"
#include "CoconutLib.h"
#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/bind.hpp>
#endif

using namespace coconut::protocol;
	
namespace coconut {

void GotProtocolControllerEvent::fireObservers(
						boost::shared_ptr<BaseController> targetController, 
						boost::shared_ptr<BaseProtocol> prot) {
	bool unlock = true;
	lockObserver_.lock();
	if(observers_.size() > 0) {
		setObservers_t observersTemp = observers_;
		lockObserver_.unlock();
		unlock = false;

		setObservers_t::iterator it = observersTemp.begin();

		for(; it != observersTemp.end(); it++) {

			if((*it)->ioService()->isCalledInMountedThread() == false) {
				(*it)->eventGotProtocol()->deferredCaller()->deferredCall(
						boost::bind(&BaseController::_onPreControllerEvent_GotProtocol, *it, targetController, prot)
						);
			} else {
				// call directly..
				(*it)->_onPreControllerEvent_GotProtocol(targetController, prot);
			}
		}
	}

	if(unlock) 
		lockObserver_.unlock();
}


void OccuredErrorControllerEvent::fireObservers(
						boost::shared_ptr<BaseController> targetController, int error) {
	
	bool unlock = true;
	lockObserver_.lock();
	if(observers_.size() > 0) {
		setObservers_t observersTemp = observers_;
		lockObserver_.unlock();
		unlock = false;

		setObservers_t::iterator it = observersTemp.begin();

		for(; it != observersTemp.end(); it++) {

			if((*it)->ioService()->isCalledInMountedThread() == false) {
				(*it)->eventOccuredError()->deferredCaller()->deferredCall(
						boost::bind(&BaseController::_onPreControllerEvent_OccuredError, *it, targetController, error)
						);
			} else {
				// call directly..
				(*it)->_onPreControllerEvent_OccuredError(targetController, error);
			}
		}
	}

	if(unlock) 
		lockObserver_.unlock();
}


void ClosedConnectionControllerEvent::fireObservers(
						boost::shared_ptr<BaseController> targetController, int error) {
	bool unlock = true;
	lockObserver_.lock();
	if(observers_.size() > 0) {
		setObservers_t observersTemp = observers_;
		lockObserver_.unlock();
		unlock = false;

		setObservers_t::iterator it = observersTemp.begin();

		for(; it != observersTemp.end(); it++) {

			if((*it)->ioService()->isCalledInMountedThread() == false) {
				(*it)->eventClosedConnection()->deferredCaller()->deferredCall(
						boost::bind(&BaseController::_onPreControllerEvent_ClosedConnection, *it, targetController, error)
						);
			} else {
				// call directly..
				(*it)->_onPreControllerEvent_ClosedConnection(targetController, error);
			}
		}
	}
	if(unlock) 
		lockObserver_.unlock();
}


void GotResponseControllerEvent::fireObservers(
						boost::shared_ptr<BaseController> targetController, int ticket) {
	bool unlock = true;
	lockObserver_.lock();
	mapTicketObservers_t::iterator it = observers_.find(ticket);
	if(it != observers_.end()) {
		if(it->second.size() > 0) {
			setObservers_t observersTemp = it->second;
			lockObserver_.unlock();
			unlock = false;

			setObservers_t::iterator it = observersTemp.begin();

			for(; it != observersTemp.end(); it++) {

				if((*it)->ioService()->isCalledInMountedThread() == false) {
					(*it)->eventGotResponse()->deferredCaller()->deferredCall(
							boost::bind(&BaseController::_onPreControllerEvent_GotResponse, *it, targetController, ticket)
							);
				} else {
					// call directly..
					(*it)->_onPreControllerEvent_GotResponse(targetController, ticket);
				}
			}
		}
	}
	if(unlock) 
		lockObserver_.unlock();
}

}
