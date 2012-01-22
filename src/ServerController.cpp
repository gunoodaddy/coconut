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
#include "Logger.h"
#include "ServerController.h"
#include "ClientController.h"
#include "IOService.h"

namespace coconut {

ServerController::~ServerController() {
	LOG_TRACE("~ServerController() : %p\n", this);
}

boost::shared_ptr<IOService> ServerController::ioService() {
	return connListener_->ioService();
}

BaseIOServiceContainer *ServerController::ioServiceContainer() {
	return connListener_->ioService()->ioServiceContainer();
}

void ServerController::processDelayedRemoveClientFromSet() {
	clientset_t::iterator it = delay_remove_clients_set_.begin();
	for(; it != delay_remove_clients_set_.end(); it++) {

		clientset_t::iterator itReal = clients_.find(*it);
		if(itReal != clients_.end()) {
			clients_.erase(itReal);
			LOG_DEBUG("remove delaying client from set\n");
		}
	}

	delay_remove_clients_set_.clear();
}

void ServerController::onConnectionListener_Accept(coconut_socket_t newSocket) {

	// Caution! *MUST* call Reactor::ioService()
	boost::shared_ptr<IOService> ioService = ioServiceContainer()->ioService();
	boost::shared_ptr<TcpSocket> newTcpSocket(new TcpSocket(ioService));

	// onAccept emitted..
	boost::shared_ptr<ClientController> newController = onAccept(newTcpSocket);
	clients_.insert(newController);

	newController->setSocket(newTcpSocket);
	newController->setReconnectable(false);
	newTcpSocket->setEventHandler(newController.get());

	// for client event..
	newController->eventClosedConnection()->registerObserver(this);

	newTcpSocket->attachSocketHandle(newSocket, false);
	newTcpSocket->install();	// for multithreading.. this method call last!
}	

void ServerController::onConnectionListener_Error(int error) { 
	onError(error);
}	

void ServerController::onTimer_Timer(int id) {
	switch(id) {
		case TIMERID_DELAYED_REMOVE:
			processDelayedRemoveClientFromSet();
			return;
	}

	BaseController::onTimer_Timer(id);
}

void ServerController::_onPreControllerEvent_OccuredError(
		boost::shared_ptr<BaseController> controller, 
		int error) {
	LOG_DEBUG("[ServerController] _onPreControllerEvent_OccuredError emitted.. error = %d\n", error);
	_onPreControllerEvent_ClosedConnection(controller, error);
}

void ServerController::_onPreControllerEvent_ClosedConnection(
		boost::shared_ptr<BaseController> controller, 
		int error) {
	LOG_DEBUG("[ServerController] _onPreControllerEvent_ClosedConnection emitted.. error = %d\n", error);
	ScopedMutexLock(lockClients_);

/*
	boost::shared_ptr<ClientController> clientController = boost::static_pointer_cast<ClientController>(controller);
	if(clientController && clientController->isReconnectable()) {
		return;
	}
*/	
	_makeTimer();
	timerObj_->setTimer(TIMERID_DELAYED_REMOVE, 10 /*msec*/, false);

	delay_remove_clients_set_.insert(controller);

	onControllerEvent_ClosedConnection(controller, error);
}

} // end of namespace coconut
