#include "Coconut.h"
#include <arpa/inet.h>
#include "NetworkHelper.h"
#include "IOServiceContainer.h"

#define UDP_PORT	7777

class MyUdpClientController : public coconut::BinaryController {
public:
	void onReceivedDatagram(const void *data, int size, const struct sockaddr_in *sin) {
		printf("onReadFrom emitted.. %s:%d from %s:%d\n", (char*)data, size, inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));
	}
};


int main(void) {
	coconut::IOServiceContainer ioServiceContainer;

	boost::shared_ptr<MyUdpClientController> udpclient(new MyUdpClientController);
	coconut::NetworkHelper::bindUdp(&ioServiceContainer, 0/*for client*/, udpclient);
	udpclient->udpSocket()->writeTo("HELLO", 5, "localhost", UDP_PORT);

	ioServiceContainer.run();
}

