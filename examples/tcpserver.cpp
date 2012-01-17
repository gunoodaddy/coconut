#include "Coconut.h"
#include <log4cxx/logger.h> 
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include "NetworkHelper.h"
#include "IOServiceContainer.h"
#include "LineController.h"

using namespace log4cxx;
LoggerPtr logger(Logger::getLogger("MyApp"));

//#define USE_LINE_CONTROLLER

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

int main(int argc, char **argv) {
	PropertyConfigurator::configure("log4cxx_tcpserver.properties");

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

		LOG4CXX_INFO(logger, "tcpserver started..");
		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		printf("Exception emitted : %s\n", e.what());
	}
	return 0;
}
