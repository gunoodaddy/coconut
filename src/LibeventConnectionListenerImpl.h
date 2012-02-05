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

#if ! defined(WIN32)
#include <sys/un.h>
#endif
#include <errno.h>
#include <event2/util.h>
#include <event2/listener.h>
#include "ConnectionListener.h"
#include "ConnectionListenerImpl.h"

namespace coconut {

class LibeventConnectionListenerImpl : public ConnectionListenerImpl {
public:
	LibeventConnectionListenerImpl(ConnectionListener *owner, int port) 
		: owner_(owner)
		, listener_(NULL)
		, path_("")
		, port_(port) { }

	LibeventConnectionListenerImpl(ConnectionListener *owner, const char* path)
		: owner_(owner)
		, listener_(NULL)
		, path_(path)
		, port_(0) { }

	~LibeventConnectionListenerImpl(void) {
		if(listener_)
			evconnlistener_free(listener_);
	}

private:
	static void accept_conn_cb(
					struct evconnlistener *listener,
					coconut_socket_t fd, struct sockaddr *address, int socklen,
					void *ctx) {
		LibeventConnectionListenerImpl *SELF = (LibeventConnectionListenerImpl *)ctx;
		SELF->fire_onConnectionListener_Accept(fd);
	}

	static void accept_error_cb(struct evconnlistener *listener, void *ctx) {
		int err = EVUTIL_SOCKET_ERROR();

		LibeventConnectionListenerImpl *SELF = (LibeventConnectionListenerImpl *)ctx;
		SELF->fire_onConnectionListener_Error(err);
	}

public:
	void listen() {
		ScopedIOServiceLock(owner_->ioService());
		if(listener_) {
			throw IllegalStateException("Already listener created");
		}
		struct sockaddr_in sin;
#if ! defined(WIN32)
		struct sockaddr_un sun;
#endif
		struct sockaddr* sinptr;
		int addrlen;

		if(isUnixDomainListenMode() == false) {
			memset(&sin, 0, sizeof(sin));
			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = htonl(0);
			sin.sin_port = htons(port_);
			sinptr = (struct sockaddr*)&sin;
			addrlen = sizeof(sin);
		} else {
#if ! defined(WIN32)
			unlink(path_.c_str());
			memset(&sin, 0, sizeof(sin));
			sun.sun_family = AF_LOCAL;
			strcpy(sun.sun_path, path_.c_str());
			sinptr = (struct sockaddr*)&sun;
			addrlen = sizeof(sun);
#else
			assert(false && "Windows not support unix domain socket..");
#endif

		}

		listener_ = evconnlistener_new_bind(owner_->ioService()->coreHandle(), accept_conn_cb, this,
				LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
				sinptr, addrlen);

		if(!listener_) {
			throw SocketException("Couldn't create listener");
		}
		evconnlistener_set_error_cb(listener_, accept_error_cb);
		_LOG_DEBUG("Connection Listener started.. port : %d", port_);
	}

	void fire_onConnectionListener_Accept(coconut_socket_t newSocket) {
		owner_->eventHandler()->onConnectionListener_Accept(newSocket);
	}

	void fire_onConnectionListener_Error(int error) {
		owner_->eventHandler()->onConnectionListener_Error(error);
	}

	const std::string & listeningPath() {
		return path_;
	}

private:
	bool isUnixDomainListenMode() {
		return listeningPath().size() > 0;
	}

private:
	ConnectionListener *owner_;

	struct evconnlistener *listener_;
	std::string path_;
	int port_;
};

}


