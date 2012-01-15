#pragma once
#include "config.h"
#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#endif
#include "BaseVirtualTransport.h"

#if defined(_MSC_VER)
#pragma warning ( disable: 4231 4251 4275 4786 )
#endif

#define SOCKET_ERR_RW_RETRIABLE(e)              \
    ((e) == EINTR || (e) == EAGAIN)

#if defined(WIN32)
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace coconut {

class IOService;

class COCONUT_API BaseSocket : public BaseVirtualTransport {
public:
	BaseSocket(boost::shared_ptr<IOService> ioService, SocketType type) : ioService_(ioService)
		, type_(type)
		, state_(Disconnected)
		, handler_(NULL) { }

	virtual ~BaseSocket() { }

	class EventHandler {
	public:
		virtual ~EventHandler() { }

		virtual void onSocket_Connected() { }
		virtual void onSocket_Error(int error, const char *strerror) { }
		virtual void onSocket_ReadEvent(int fd) { }
		//virtual void onSocket_Read(const void *data, int size) { }
		virtual void onSocket_ReadFrom(const void *data, int size, struct sockaddr_in * sin) { }
		virtual void onSocket_Close() { }
	};

	enum SocketState {
		Connecting,
		Connected,
		Disconnected
	};


public:
	const char *className() {
		return "BaseSocket";
	}

	void setLastErrorString(const char *errorStr) {
		lastErrorString_ = errorStr;
	}

	const char *lastErrorString() {
		return lastErrorString_.c_str();
	}

	virtual int socketFD() { 
		return -1;
	}

	bool isConnected() {
		return state_ == Connected;
	}

	boost::shared_ptr<IOService> ioService() {
		return ioService_;
	}

	SocketType type() {
		return type_;
	}

	void setEventHandler(EventHandler *handler) {
		handler_ = handler;
	}

	EventHandler *eventHandler() {
		return handler_;
	}

	void setState(SocketState state) {
		state_ = state;
	}

	SocketState state() {
		return state_;
	}

	virtual void close() {
		assert(0 && "BaseSocket::close can not be called directly");
	}

	virtual void checkResponseSocket(int res) {
		assert(0 && "BaseSocket::checkResponseSocket can not be called directly");
	}

public:
	virtual int read(std::string &data, size_t size) {
		assert(0 && "BaseSocket::read(std::string) can not be called directly");
		return -1;
	}
	virtual int read(void *data, size_t size) { 
		assert(0 && "BaseSocket::read(void *) can not be called directly");
		return -1;
	}
	virtual int write(const void *data, size_t size) {
		assert(0 && "BaseSocket::write can not be called directly");
		return -1;
	}

protected:
	boost::shared_ptr<IOService> ioService_;	
	SocketType type_;
	SocketState state_;
	EventHandler *handler_;
	std::string lastErrorString_;
};

}
