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
#include "InternalLogger.h"
#include "ClientController.h"
#include "BufferedTransport.h"
#include "IOService.h"

namespace coconut {

ClientController::~ClientController() {
	_LOG_TRACE("~ClientController() : %p\n", this);
}

boost::shared_ptr<IOService> ClientController::ioService() {
	return socket_->ioService();
}

BaseIOServiceContainer* ClientController::ioServiceContainer() {
	return socket_->ioService()->ioServiceContainer();
}

boost::shared_ptr<ClientController> ClientController::sharedMyself() {
	return boost::static_pointer_cast<ClientController>(shared_from_this());
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
				_LOG_TRACE("New Protocol make #1 in %p\n", this);
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
			_LOG_TRACE("New Protocol make #1 in %p\n", this);
			protocol_ = protocolFactory_->makeProtocol();
		}

		_LOG_DEBUG("ClientController read socket fd = %d, readSize = %d in %p\n", socket()->socketFD(), nread, this); 
		protocol_->addToReadingBuffer(chunk, nread);
		do{
			if(protocol_->processReadFromReadingBuffer() == true) {
				// fire!
				onReceivedProtocol(protocol_);
				eventGotProtocol()->fireObservers(shared_from_this(), protocol_);

#define ALWAS_MAKE_PROTOCOL
#ifdef ALWAS_MAKE_PROTOCOL
				_LOG_TRACE("ClientController Protocol receved completed. readSize = %d, remainBufferSize = %d in %p\n", 
							nread, protocol_->remainingBufferSize(), this);

				if(protocol_->remainingBufferSize() > 0) {
					// new protocol
					_LOG_TRACE("New Protocol make #2 in %p\n", this);
					boost::shared_ptr<protocol::BaseProtocol> protocolTemp = protocolFactory_->makeProtocol();
					protocolTemp->addToReadingBuffer(protocol_->remainingBufferPtr(), protocol_->remainingBufferSize());
					protocol_ = protocolTemp;
#else
					protocol_->resetReadingBufferToRemainingBuffer();
#endif
					continue;
				} else {
					break;
				}
			} 

			// parsing failed..
			
			if(protocol_->isInvalidPacketReceived()) {
				// this session close!
				_LOG_ERROR("Invalid Packet Recved.. this = %p, size = \n", this, protocol_->payloadBuffer()->totalSize());
				socket()->close();
				break;
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
			_LOG_TRACE("New Protocol make\n");
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
