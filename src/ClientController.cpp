#include "Coconut.h"
#include "Logger.h"
#include "ClientController.h"
#include "BufferedTransport.h"
#include "IOService.h"

namespace coconut {

ClientController::~ClientController() {
	LOG_TRACE("~ClientController() : %p\n", this);
}

boost::shared_ptr<IOService> ClientController::ioService() {
	return socket_->ioService();
}

BaseIOServiceContainer* ClientController::ioServiceContainer() {
	return socket_->ioService()->ioServiceContainer();
}


void ClientController::setReconnectable(bool enable) {
	ScopedIOServiceLock(ioService());
	reconnectable_ = enable;

	if(false == reconnectable_) {
		if(timerObj_)
			timerObj_->killTimer(TIMERID_RECONNECT);
	}
}

void ClientController::processReconnect() {
	if(reconnectable_) {
		// TODO reconnect need details => jitter, noise check
		_makeTimer();
		timerObj_->setTimer(TIMERID_RECONNECT, 1000);
	}
}

void ClientController::onSocket_Initialized() {
	fireOnInitialized();
}

void ClientController::onSocket_Connected() { 
	onConnected();
}

void ClientController::onSocket_Error(int error, const char*strerror) {
	onError(error, strerror);

	processReconnect();

	eventClosedConnection()->fireObservers(shared_from_this(), error);
}

void ClientController::onSocket_Close() {
	onClosed();

	processReconnect();

	eventClosedConnection()->fireObservers(shared_from_this(), 0);
}

void ClientController::onSocket_ReadEvent(int fd) { 
	if(protocolFactory_) {
//#define PROTOCOL_READ_FROM_SOCKET
#ifdef PROTOCOL_READ_FROM_SOCKET
		do {
			if(!protocol_ || protocol_->isReadComplete()) {
				LOG_TRACE("New Protocol make #1 in %p\n", this);
				protocol_ = protocolFactory_->makeProtocol();
			}

			if(protocol_->processRead(socket()) == true) {
				onReceivedProtocol(protocol_);
			} else {
				break;
			}
		} while(1);
#else
		char chunk[IOBUF_LEN];
		int nread = socket()->read(chunk, IOBUF_LEN);
		if(nread <= 0)
			return;

		if(!protocol_ || protocol_->isReadComplete()) {
			LOG_TRACE("New Protocol make #1 in %p\n", this);
			protocol_ = protocolFactory_->makeProtocol();
		}

		LOG_DEBUG("ClientController read socket fd = %d, readSize = %d in %p\n", socket()->socketFD(), nread, this); 
		protocol_->addToReadingBuffer(chunk, nread);
		do{
			if(protocol_->processReadFromReadingBuffer() == true) {
				onReceivedProtocol(protocol_);
				eventGotProtocol()->fireObservers(shared_from_this(), protocol_);

#define ALWAS_MAKE_PROTOCOL
#ifdef ALWAS_MAKE_PROTOCOL
				LOG_TRACE("ClientController read socket readSize = %d, remainBufferSize = %d in %p\n", 
							nread, protocol_->remainingBufferSize(), this);

				if(protocol_->remainingBufferSize() > 0) {
					// new protocol
					LOG_TRACE("New Protocol make #2 in %p\n", this);
					boost::shared_ptr<protocol::BaseProtocol> protocolTemp = protocolFactory_->makeProtocol();
					protocolTemp->addToReadingBuffer(protocol_->remainingBufferPtr(), protocol_->remainingBufferSize());
					protocol_ = protocolTemp;
#else
					protocol_->resetReadingBufferToRemainingBuffer();
#endif
				} else {
					break;
				}
			} else {
				break;
			}
		}while(1);
#endif
	} else {
		char buffer[IOBUF_LEN];
		int res = socket()->read(buffer, IOBUF_LEN);
		if(res > 0)
			onReceivedData(buffer, res);
	}
}

void ClientController::onSocket_ReadFrom(const void *data, int size, struct sockaddr_in * sin) {
	if(protocolFactory_) {
		if(!protocol_ || protocol_->isReadComplete()) {
			protocol_ = protocolFactory_->makeProtocol();
			LOG_TRACE("New Protocol make\n");
		}

		// already received from socket to "data" buffer..
		protocol_->resetBuffer();
		protocol_->addToReadingBuffer(data, size);
		if(protocol_->processReadFromReadingBuffer() == true) {
			onReceivedProtocol(protocol_);
		}
	} else {
		onReceivedDatagram(data, size, sin);
	}
}

void ClientController::onTimer_Timer(int id) {
	if(id == TIMERID_RECONNECT) {
		timerObj_->killTimer(id);

		// TODO : retry max check..
		retryConnectCnt_++;

		tcpSocket()->connect();
		return;
	}

	BaseController::onTimer_Timer(id);
}


} // end of namespace coconut
