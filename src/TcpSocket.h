#pragma once

#include "BaseSocket.h"

struct kbuffer;

namespace coconut {

class IOService;
class TcpSocketImpl;

class COCONUT_API TcpSocket : public BaseSocket {
public:
	TcpSocket(boost::shared_ptr<IOService> ioService);
	~TcpSocket();

public:
	int socketFD();
	void connect();
	void connect(const char *host, int port, int timeout = 0);
	void connectUnix(const char *path, int timeout = 0);
	void attachSocketHandle(coconut_socket_t fd, bool doInstallFlag = true);
	int read(void *data, size_t size);
	int read(std::string &data, size_t size);
	int write(const void *data, size_t size);
	void close();
	void checkResponseSocket(int res);
	void install();

private:
	TcpSocketImpl *impl_;
};

}
