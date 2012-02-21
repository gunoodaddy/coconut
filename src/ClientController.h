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

#include "BaseSocket.h"
#include "BaseProtocol.h"
#include "TcpSocket.h"
#include "UdpSocket.h"
#include "BaseController.h"

namespace coconut {

class IOService;

class COCONUT_API ClientController : public BaseController
                       , public BaseSocket::EventHandler
{
public:
	static const int TIMERID_RECONNECT = (1|INTERNAL_TIMER_BIT);
public:
	ClientController() : ioServiceContainer_(NULL), reconnectable_(true), initFlag_(false), retryConnectCnt_(0) {
	}

	virtual ~ClientController();

public:
	// strategy pattern
	void setSocket(boost::shared_ptr<BaseSocket> socket) {
		socket_ = socket;
	}

	void setProtocolFactory(boost::shared_ptr<protocol::BaseProtocolFactory> factory) {
		protocolFactory_ = factory;
	}

	boost::shared_ptr<protocol::BaseProtocolFactory> protocolFactory() {
		return protocolFactory_;
	}

	boost::shared_ptr<ClientController> sharedMyself();

	boost::shared_ptr<BaseSocket> socket() {
		return socket_;
	}

	boost::shared_ptr<TcpSocket> tcpSocket() {
		assert(socket_->type() == TCP && "invalid socket type : TCP");

		return boost::static_pointer_cast<TcpSocket>(socket_);
	}

	boost::shared_ptr<UdpSocket> udpSocket() {
		assert(socket_->type() == UDP && "invalid socket type : UDP");

		return boost::static_pointer_cast<UdpSocket>(socket_);
	}

	boost::shared_ptr<IOService> ioService();
	BaseIOServiceContainer* ioServiceContainer();

	void setReconnectable(bool enable);
	bool isReconnectable() {
		return reconnectable_;
	}

	void fireOnInitialized() {
		if(false == initFlag_) {
			initFlag_ = true;
			_onPreInitialized();
		}
	}

	void fire_onClosed();
	void fire_onError(int error, std::string strerror);

private:
	void processReconnect();

private:
	void onSocket_Initialized();
	void onSocket_Connected();
	void onSocket_Error(int error, const char*strerror);
	void onSocket_Close();
	void onSocket_ReadFrom(const void *data, int size, struct sockaddr_in * sin);
	void onSocket_ReadEvent(coconut_socket_t fd);
	void onTimer_Timer(int id);

protected:
	// ClientController callback event

	virtual void onReceivedProtocol(boost::shared_ptr<protocol::BaseProtocol> protocol) { }
	virtual void onReceivedData(const void *data, int size) { }
	virtual void onReceivedDatagram(const void *data, int size, const struct sockaddr_in *sin) { }
	virtual void onConnected() { }
	virtual void onClosed() { }
	virtual void onError(int error, const char *strerror) { }

	
protected:
	BaseIOServiceContainer *ioServiceContainer_;
	boost::shared_ptr<BaseSocket> socket_;
	boost::shared_ptr<protocol::BaseProtocol> protocol_;
	bool reconnectable_;
	bool initFlag_;
	int retryConnectCnt_;

	boost::shared_ptr<protocol::BaseProtocolFactory> protocolFactory_;
};

class COCONUT_API BinaryController : public ClientController {	// TODO : divide this class COCONUT_API to another file..
private:
};


} // end of namespace coconut
