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

#include "Coconut.h"
#include "UdpSocket.h"
#include "IOService.h"
#include "Exception.h"
#include "IPv4Address.h"
#include <errno.h>
#if defined(WIN32)
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif
#include <event2/event.h>
#include "DNSResolver.h"
#include "InternalLogger.h"

namespace coconut {

class UdpSocketImpl : public DNSResolver::EventHandler {
public:
	UdpSocketImpl(UdpSocket *owner, int port) 
		: owner_(owner)
		, ev_(NULL)
		, port_(port)
		, dnsReolver_(NULL) {

		_LOG_TRACE("UdpSocketImpl() : %p", this);
	}

	~UdpSocketImpl() {
		close();

		if(dnsReolver_)
			delete dnsReolver_;

		_LOG_TRACE("~UdpSocketImpl() : %p", this);
	}

private:
	struct write_context_t {
		void *data_alloc;
		int size;
		int port;
	};

private:
	static void event_cb(coconut_socket_t fd, short what, void *arg) {
		UdpSocketImpl *SELF = (UdpSocketImpl *)arg;
		CHECK_IOSERVICE_STOP_VOID_RETURN(SELF->ioService());

		if(what & EV_READ) {
			char buf[UDP_BUF_SIZE] = {0, };
			struct sockaddr_in client_addr;
			socklen_t sizeaddr = sizeof(client_addr);

			int len = ::recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, &sizeaddr);   

			SELF->fire_onSocket_ReadFrom(buf, len, &client_addr);
		}
	}
	
public:
	boost::shared_ptr<IOService> ioService() {
		return owner_->ioService();
	}

	coconut_socket_t socketFD() {
		if(ev_)
			return event_get_fd(ev_);
		return COOKIE_INVALID_SOCKET;
	}

	void connect() {
		ScopedIOServiceLock(owner_->ioService());
		if(ev_)
			throw IllegalStateException("already event object is created");

		if(::connect(event_get_fd(ev_), (struct sockaddr *)&sin_, sizeof(sin_)) < 0)
			throw SocketException("Error connecting datagram socket");

		owner_->setState(BaseSocket::Connected);
	}

	void bind() {
		ScopedIOServiceLock(owner_->ioService());
		memset(&sin_, 0, sizeof(sin_));
		sin_.sin_family = AF_INET;
		sin_.sin_addr.s_addr = INADDR_ANY;
		sin_.sin_port = htons(port_);

		int sock;
		int yes = 1;
		if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
			throw SocketException("Error creating datagram socket");

		if (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(int)) < 0)
			throw SocketException("Error setting datagram socket option");

		if(0 != port_) {
			_LOG_INFO("UDP BIND START : port = %d\n", port_);
			if (::bind(sock, (struct sockaddr*)&sin_, sizeof(struct sockaddr)) < 0) {
				throw SocketException("Error binding datagram socket");
			}
		}

		ev_ = event_new(owner_->ioService()->coreHandle(), sock, EV_READ|EV_PERSIST, event_cb, this);
		event_add(ev_, NULL);	// TODO UDP READ TIMEOUT??

		owner_->eventHandler()->onSocket_Initialized();
	}

	void close() {
		ScopedIOServiceLock(owner_->ioService());
		if(ev_) {
			evutil_closesocket(event_get_fd(ev_));
			event_free(ev_);
			ev_ = NULL;
		}
	}

	const BaseAddress * peerAddress() {
		peerAddress_.setSocketAddress(&lastclient_sin_);
		return &peerAddress_;
	}

	const BaseAddress * sockAddress() {
		sockAddress_.setSocketAddress(&sin_);
		return &sockAddress_;
	}

	void checkResponseSocket(int res) {
		if (res == -1) {
			int err = evutil_socket_geterror(socketFD());
			if (!SOCKET_ERR_RW_RETRIABLE(err)) {
				
	//			fire_onSocket_Error(err); // TODO
				close();
			}
		} else if (res == 0) {
			/* eof case */
	//		fire_onSocket_Close(); // TODO
			close();
		}
	}

	void onDnsResolveResult(int errcode, const char *host, struct addrinfo *addr, void *ptr) {
		struct write_context_t *context = (struct write_context_t *)ptr;	

		if (errcode) {
			_LOG_DEBUG("DNS Resolve Error : %s -> %s\n", host, evutil_gai_strerror(errcode));
		} else {
			struct addrinfo *ai;
			int resCnt = 0;
			for (ai = addr; ai; ai = ai->ai_next)
				resCnt++;

			int index = rand() % resCnt;
			int i = 0;
			for (ai = addr; ai; ai = ai->ai_next, i++) {
				if(index != i)
					continue;

				if (ai->ai_family == AF_INET) {
					struct sockaddr_in *sin = (struct sockaddr_in *)ai->ai_addr;
					sin->sin_port = htons(context->port);
					writeTo(context->data_alloc, context->size, sin);
				} else if (ai->ai_family == AF_INET6) {
					// TODO : DNS resolve ipv6 NOT SUPPORTED YET..
					//struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ai->ai_addr;
					//s = evutil_inet_ntop(AF_INET6, &sin6->sin6_addr, buf, 128);
				}
				break;
			}
			evutil_freeaddrinfo(addr);
		}

		free(context->data_alloc);
		free(context);
	}

	int writeTo(const void *data, size_t size, const struct sockaddr_in *sin) {
		assert(ev_ && "socket is not initailized");
		_LOG_INFO("WRITETO : %s:%d => [%d]\n", inet_ntoa(sin->sin_addr), ntohs(sin->sin_port), size);
		int len = sendto(event_get_fd(ev_), (const char *)data, size, 0, (struct sockaddr *)sin, sizeof(struct sockaddr_in));
		return len;
	}

	int writeTo(const void *data, size_t size, const char *host, int port) {
		assert(ev_ && "socket is not initailized");
		struct sockaddr_in sin;
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_port = htons(port);
		if(evutil_inet_pton(AF_INET, host, &(sin.sin_addr)) > 0) {
			writeTo(data, size, &sin);
		} else {
			ScopedIOServiceLock(owner_->ioService());
			if(NULL == dnsReolver_)
				dnsReolver_ = new DNSResolver(owner_->ioService());

			struct write_context_t *context = (struct write_context_t*)malloc(sizeof(struct write_context_t));
			context->data_alloc = malloc(size);
			memcpy(context->data_alloc, data, size);
			context->size = size;
			context->port = port;

			if(dnsReolver_->resolve(host, &sin, this, context) == false) {
				free(context->data_alloc);
				free(context);
				return -1;
			}
		}
		return 0;
	}

	int write(const void *data, size_t size) {
		assert(ev_ && "socket is not initailized");
		int len = sendto(event_get_fd(ev_), (const char *)data, size, 0, (struct sockaddr *)&lastclient_sin_, sizeof(lastclient_sin_));
		return len;
	}

	void fire_onSocket_ReadFrom(const void *data, int size, struct sockaddr_in * sin) { 
		lastclient_sin_ = *sin;
		owner_->eventHandler()->onSocket_ReadFrom(data, size, sin);
	}

private:
	UdpSocket *owner_;
	struct event *ev_;
	int port_;
	struct sockaddr_in lastclient_sin_;
	DNSResolver *dnsReolver_;
	struct sockaddr_in sin_;
	std::string currentIP_;

	IPv4Address peerAddress_;
	IPv4Address sockAddress_;
};

//-------------------------------------------------------------------------------------------------

UdpSocket::UdpSocket(boost::shared_ptr<IOService> ioService, int port) : BaseSocket(ioService, UDP) {
	impl_ = new UdpSocketImpl(this, port);
}

UdpSocket::~UdpSocket() {
	delete impl_;
}

int UdpSocket::socketFD() {
	return impl_->socketFD();
}

void UdpSocket::connect() {
	impl_->connect();
}

void UdpSocket::bind() {
	impl_->bind();
}

void UdpSocket::close() {
	impl_->close();
}

void UdpSocket::checkResponseSocket(int res) {
	impl_->checkResponseSocket(res);
}

int UdpSocket::writeTo(const void *data, size_t size, const struct sockaddr_in *sin) {
	return impl_->writeTo(data, size, sin);
}

int UdpSocket::writeTo(const void *data, size_t size, const char *host, int port) {
	return impl_->writeTo(data, size, host, port);
}

int UdpSocket::write(const void *data, size_t size) {
	return impl_->write(data, size);
}

const BaseAddress * UdpSocket::peerAddress() {
	return impl_->peerAddress();
}

const BaseAddress * UdpSocket::sockAddress() {
	return impl_->sockAddress();
}

}
