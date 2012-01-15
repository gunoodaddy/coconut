#include "Coconut.h"
#include <arpa/inet.h>
#include "NetworkHelper.h"
#include "IOServiceContainer.h"

#define UDP_PORT	7777

class MyUdpClientController : public coconut::BinaryController {
public:
	virtual void onInitialized() {
		setTimer(1, 1000, true);
	}

	void onReceivedDatagram(const void *data, int size, const struct sockaddr_in *sin) {
		printf("onReadFrom emitted.. %d from %s:%d\n", size, inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));
		udpSocket()->writeTo(data, size, sin);
	}

	virtual void onTimer(unsigned short id) {
	}
};

int main(void) {
	coconut::IOServiceContainer ioServiceContainer;
	ioServiceContainer.initialize();

	try {
		// udp server test
		boost::shared_ptr<MyUdpClientController> udpController(new MyUdpClientController);
		coconut::NetworkHelper::bindUdp(&ioServiceContainer, UDP_PORT, udpController);

		// event loop start!
		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		printf("Exception emitted : %s\n", e.what());
	}

	// exit..
}

