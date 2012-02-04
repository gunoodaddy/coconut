// TcpEchoServer.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "Coconut.h"

#define USE_LINE_CONTROLLER

#ifdef USE_LINE_CONTROLLER
class MyClientController : public coconut::LineController {
public:
	virtual void onLineReceived(const char *line) {
		writeLine(line);
	}
};
#else
class MyClientController : public coconut::BinaryController {
	virtual void onReceivedData(const void *data, int size) {
		socket()->write(data, size);	
	}
};
#endif

class MyServerController : public coconut::ServerController {
	virtual boost::shared_ptr<coconut::ClientController> onAccept(boost::shared_ptr<coconut::TcpSocket> socket) {
		boost::shared_ptr<MyClientController> newController(new MyClientController); 
		return newController;
	}
};


int _tmain(int argc, _TCHAR* argv[]) {
	int port = 8000;

	WSADATA wsaData;
	::WSAStartup(MAKEWORD(2, 2), &wsaData);

	coconut::setEnableDebugMode();

	coconut::IOServiceContainer ioServiceContainer;

	ioServiceContainer.turnOnIOCP(4);
	ioServiceContainer.initialize();
	
	try {
		boost::shared_ptr<MyServerController> serverController(new MyServerController);

		coconut::NetworkHelper::listenTcp(&ioServiceContainer, port, serverController);

		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		printf("Exception emitted : %s\n", e.what());
	}
	
	return 0;
}
