#pragma once

#include "BaseSocket.h"

#if defined(WIN32)
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

#define UDP_BUF_SIZE	8096

namespace coconut {

class IOService;
class UdpSocketImpl;

class COCONUT_API UdpSocket : public BaseSocket {
public:
	UdpSocket(boost::shared_ptr<IOService> ioService, int port);
	~UdpSocket();
	
public:
	int socketFD();
	void connect();
	void bind();
	void close();
	void checkResponseSocket(int res);

	int writeTo(const void *data, size_t size, const struct sockaddr_in *sin);
	int writeTo(const void *data, size_t size, const char *host, int port);
	int write(const void *data, size_t size);

	const struct sockaddr_in * lastClientAddress();

private:
	UdpSocketImpl *impl_;
};

}
