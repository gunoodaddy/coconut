#include "Coconut.h"
#include "ServerController.h"
#include "ClientController.h"
#include "IOService.h"

namespace coconut {

ServerController::~ServerController() {
	LOG_TRACE("~ServerController()\n");
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
			LOG_DEBUG("DELAYED CLIENT REMOVED FROM SET..\n");
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

	newController->setSocket(newTcpSocket);
	newTcpSocket->setEventHandler(newController.get());

	// for client event..
	newController->eventClosedConnection()->registerObserver(this);

	newTcpSocket->attachSocketHandle(newSocket, false);	// for multithreading.. this method call last!
	newTcpSocket->install();

	clients_.insert(newController);
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
					boost::shared_ptr<coconut::BaseController> controller, 
					int error) {
	_onPreControllerEvent_ClosedConnection(controller, error);
}

void ServerController::_onPreControllerEvent_ClosedConnection(
					boost::shared_ptr<coconut::BaseController> controller, 
					int error) {
	LOG_DEBUG("ServerController _onPreControllerEvent_ClosedConnection emitted.. error = %d\n", error);
	ScopedMutexLock(lockClients_);

	_makeTimer();
	timerObj_->setTimer(TIMERID_DELAYED_REMOVE, 10, false);

	delay_remove_clients_set_.insert(controller);

	onControllerEvent_ClosedConnection(controller, error);
}

} // end of namespace coconut
