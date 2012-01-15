#include "Coconut.h"
#include "NetworkHelper.h"
#include "IOServiceContainer.h"

class MyClientController : public coconut::BinaryController {
public:
	virtual void onError(int error, const char *strerror) {
		printf("onError emitted..\n");
	}

	virtual void onClosed() { 
		printf("onClose emitted..\n");
	}

	virtual void onConnected() {
		printf("onConnected emitted..\n");
		socket()->write((const void *)"HELLO\r\n", 7);
	}

	virtual void onTimer(unsigned short id) {
		printf("onTimer emitted.. %d\n", id);
	}
	
	virtual void onReceivedData(const void *data, int size) {
		printf("onRead emitted.. %d\n", size);
	}
};

int main(void) {
	coconut::IOServiceContainer ioServiceContainer;
	ioServiceContainer.initialize();

	try {
		// unix client test
		boost::shared_ptr<MyClientController> unixClientController(new MyClientController);
		coconut::NetworkHelper::connectUnix(&ioServiceContainer, "udsserver.sock", unixClientController);

		// event loop start!
		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		printf("Exception emitted : %s\n", e.what());
	}

	// exit..
}

