/*
* Copyright (c) 2012, Eunhyuk Kim(gunoodaddy) 
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*   * Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*   * Neither the name of Redis nor the names of its contributors may be used
*     to endorse or promote products derived from this software without
*     specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

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
#include "BaseAddress.h"
#include "ThreadUtil.h"

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

		virtual void onSocket_Initialized() { }
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

	virtual coconut_socket_t socketFD() { 
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
		lockHandler_.lock();
		handler_ = handler;
		lockHandler_.unlock();
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

	virtual const BaseAddress * peerAddress() {
		assert(0 && "BaseSocket::peerAddress can not be called directly");
		return NULL;
	}

	virtual const BaseAddress * sockAddress() {
		assert(0 && "BaseSocket::sockAddress can not be called directly");
		return NULL;
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
	virtual int peek(char *buffer, size_t size) {
		assert(0 && "BaseSocket::peek can not be called directly");
		return 0;
	}
	virtual const void * peek(size_t &size) {
		assert(0 && "BaseSocket::peek can not be called directly");
		size = 0;
		return NULL;
	}
	virtual void throwAway(size_t size) {
		assert(0 && "BaseSocket::throwAway can not be called directly");
	}
public:
	void fire_onSocket_Initialized();
	void fire_onSocket_Connected();
	void fire_onSocket_Error(int error, const char *strerror);
	void fire_onSocket_ReadEvent(int fd);
	void fire_onSocket_ReadFrom(const void *data, int size, struct sockaddr_in * sin);
	void fire_onSocket_Close();

protected:
	boost::shared_ptr<IOService> ioService_;	
	SocketType type_;
	SocketState state_;
	EventHandler *handler_;
	std::string lastErrorString_;
	Mutex lockHandler_;
};

}
