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
#include "NetworkHelper.h"
#include "IOService.h"
#include "Timer.h"
#include "BaseController.h"
#include "BaseIOServiceContainer.h"
#include "TcpSocket.h"
#include "UdpSocket.h"
#include "ConnectionListener.h"
#include "ClientController.h"
#include "ServerController.h"

namespace coconut {

void NetworkHelper::connectTcp( BaseIOServiceContainer *ioServiceContainer,
                                const char* host, 
                                int port, boost::shared_ptr<ClientController> controller, 
                                int timeout) {

	boost::shared_ptr<TcpSocket> tcpSocket = TcpSocket::makeSharedPtr();
	tcpSocket->initialize(ioServiceContainer->ioServiceByRoundRobin());
	tcpSocket->setEventHandler(controller.get());
	controller->setSocket(tcpSocket);
	tcpSocket->connect(host, port, timeout);
}

void NetworkHelper::connectUnix( BaseIOServiceContainer *ioServiceContainer,
                                 const char* path, 
                                 boost::shared_ptr<ClientController> controller, 
                                 int timeout) {

	boost::shared_ptr<TcpSocket> tcpSocket = TcpSocket::makeSharedPtr();
	tcpSocket->initialize(ioServiceContainer->ioServiceByRoundRobin());
	tcpSocket->setEventHandler(controller.get());
	controller->setSocket(tcpSocket);
	tcpSocket->connectUnix(path, timeout);
}

void NetworkHelper::attachTcp( boost::shared_ptr<IOService> ioService,
							   coconut_socket_t sock, 
							   boost::shared_ptr<ClientController> controller) {

	boost::shared_ptr<TcpSocket> tcpSocket = TcpSocket::makeSharedPtr();
	tcpSocket->initialize(ioService);
	tcpSocket->setEventHandler(controller.get());
	controller->setSocket(tcpSocket);
	controller->setReconnectable(false);
	tcpSocket->attachSocketHandle(sock);
}


void NetworkHelper::listenTcp( BaseIOServiceContainer *ioServiceContainer,
                               int port, 
                               boost::shared_ptr<ServerController> controller) {

	boost::shared_ptr<ConnectionListener> connListener = ConnectionListener::makeSharedPtr();
	connListener->initialize(ioServiceContainer->ioServiceByRoundRobin(), port);
	controller->setConnectionListener(connListener);
	connListener->listen();
}

void NetworkHelper::listenUnix( BaseIOServiceContainer *ioServiceContainer,
                                const char *path, 
                                boost::shared_ptr<ServerController> controller) {

	boost::shared_ptr<ConnectionListener> connListener = ConnectionListener::makeSharedPtr();
	connListener->initialize(ioServiceContainer->ioServiceByRoundRobin(), path);
	controller->setConnectionListener(connListener);
	connListener->listen();
}

void NetworkHelper::bindUdp( BaseIOServiceContainer *ioServiceContainer, 
                             int port, 
                             boost::shared_ptr<ClientController> controller) {

	boost::shared_ptr<UdpSocket> udpSocket = UdpSocket::makeSharedPtr();
	udpSocket->initialize(ioServiceContainer->ioServiceByRoundRobin(), port);
	udpSocket->setEventHandler(controller.get());
	controller->setSocket(udpSocket);
	udpSocket->bind();
}

}
