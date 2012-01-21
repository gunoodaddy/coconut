#include "Coconut.h"
#include "NetworkHelper.h"
#include "IOService.h"
#include "Timer.h"
#include "BaseController.h"
#include "BaseIOServiceContainer.h"
#include "TcpSocket.h"
#include "UdpSocket.h"
#include "ConnectionListener.h"
#include "RedisRequest.h"
#include "HttpClient.h"
#include "ClientController.h"
#include "ServerController.h"

namespace coconut {

void NetworkHelper::connectTcp( BaseIOServiceContainer *ioServiceContainer,
                                const char* host, 
                                int port, boost::shared_ptr<ClientController> controller, 
                                int timeout) {

	boost::shared_ptr<TcpSocket> tcpSocket(new TcpSocket(ioServiceContainer->ioService()));
	tcpSocket->setEventHandler(controller.get());
	controller->setSocket(tcpSocket);
	tcpSocket->connect(host, port, timeout);
}

void NetworkHelper::connectUnix( BaseIOServiceContainer *ioServiceContainer,
                                 const char* path, 
                                 boost::shared_ptr<ClientController> controller, 
                                 int timeout) {

	boost::shared_ptr<TcpSocket> tcpSocket(new TcpSocket(ioServiceContainer->ioService()));
	tcpSocket->setEventHandler(controller.get());
	controller->setSocket(tcpSocket);
	tcpSocket->connectUnix(path, timeout);
}

void NetworkHelper::listenTcp( BaseIOServiceContainer *ioServiceContainer,
                               int port, 
                               boost::shared_ptr<ServerController> controller) {

	boost::shared_ptr<ConnectionListener> connListener(new ConnectionListener(ioServiceContainer->ioService(), port));
	controller->setConnectionListener(connListener);
	connListener->listen();
}

void NetworkHelper::listenUnix( BaseIOServiceContainer *ioServiceContainer,
                                const char *path, 
                                boost::shared_ptr<ServerController> controller) {

	boost::shared_ptr<ConnectionListener> connListener(new ConnectionListener(ioServiceContainer->ioService(), path));
	controller->setConnectionListener(connListener);
	connListener->listen();
}

void NetworkHelper::bindUdp( BaseIOServiceContainer *ioServiceContainer, 
                             int port, 
                             boost::shared_ptr<ClientController> controller) {

	boost::shared_ptr<UdpSocket> udpSocket(new UdpSocket(ioServiceContainer->ioService(), port));
	udpSocket->setEventHandler(controller.get());
	controller->setSocket(udpSocket);
	udpSocket->bind();
}

}
