#include "Coconut.h"
#include "BaseControllerEvent.h"
#include "BaseController.h"
#include "IOService.h"
#include "Coconut.h"
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
				(*it)->eventGotProtocol()->deferredCaller().deferredCall(
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
				(*it)->eventOccuredError()->deferredCaller().deferredCall(
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
				(*it)->eventClosedConnection()->deferredCaller().deferredCall(
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
					(*it)->eventGotResponse()->deferredCaller().deferredCall(
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
