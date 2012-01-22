#include "Coconut.h"
#include "NetworkHelper.h"
#include "IOServiceContainer.h"
#include "Logger.h"

class MyClientController : public coconut::BinaryController {
	virtual void onReceivedData(const void *data, int size) {
		socket()->write(data, size);	
	}
};

class MyServerController : public coconut::ServerController {
	virtual boost::shared_ptr<coconut::ClientController> onAccept(boost::shared_ptr<coconut::TcpSocket> socket) {
		boost::shared_ptr<MyClientController> newController(new MyClientController); 
		return newController;
	}
};

int main(int argc, char **argv) {
	if(argc < 3) {
		printf("usage : %s [port] [thread-count] [verbose:1,0]\n", argv[0]);
		return -1;
	}
	int port = atoi(argv[1]);
	int threadCount = atoi(argv[2]);
	if(argc > 3 && atoi(argv[3]) == 1)
		coconut::setEnableDebugMode();

	coconut::IOServiceContainer ioServiceContainer(threadCount);
	ioServiceContainer.initialize();
	try {
		boost::shared_ptr<MyServerController> serverController(new MyServerController);

		coconut::NetworkHelper::listenTcp(&ioServiceContainer, port, serverController);

		LOG_INFO("tcpserver started..");
		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		LOG_FATAL("Exception emitted : %s\n", e.what());
	}
	return 0;
}
