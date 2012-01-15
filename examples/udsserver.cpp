#include "Coconut.h"
#include "NetworkHelper.h"
#include "IOServiceContainer.h"

class MyClientController : public coconut::BinaryController {

	virtual void onReceivedData(const void *data, int size) { 
		printf("Recv data %d\n", size);
	}
};


class MyUnixServerController : public coconut::ServerController {
public:
	virtual boost::shared_ptr<coconut::ClientController> onAccept(boost::shared_ptr<coconut::TcpSocket> socket) {
		printf("NEW CLIENT ACCEPTED!\n");
		boost::shared_ptr<MyClientController> newController(new MyClientController); 
		return newController;
	}
};


int main(void) {
	coconut::IOServiceContainer ioServiceContainer;
	ioServiceContainer.initialize();

	try {
		// unix server test
		boost::shared_ptr<MyUnixServerController> unixServerController(new MyUnixServerController);
		coconut::NetworkHelper::listenUnix(&ioServiceContainer, "udsserver.sock", unixServerController);

		// event loop start!
		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		printf("Exception emitted : %s\n", e.what());
	}

	// exit..
}

