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

#include "dep/kbuffer.h"
#include "Timer.h"
#include <event2/dns.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <errno.h>
#if ! defined(WIN32)
#include <arpa/inet.h>
#include <sys/un.h>
#endif
#include "IPv4Address.h"
#include "TcpSocket.h"
#include "TcpSocketImpl.h"

#define _KBUFFER_ 1

namespace coconut {

class LibeventTcpSocketImpl : public TcpSocketImpl, private Timer::EventHandler {
public:
	static const int TIMERID_CONNECT = (1|INTERNAL_TIMER_BIT);

	LibeventTcpSocketImpl(TcpSocket *owner) : owner_(owner)
		, expired_(false)
		, errorDetected_(false)
		, pendingWriteSupported_(true)
		, bev_(NULL)
		, ev_read_(NULL)
		, ev_write_(NULL)
		, host_("")
		, port_(0)
		, timeout_(0)
		, write_evbuffer_(NULL)
		, write_kbuffer_(NULL)
		, dnsbase_(NULL)
		, conn_timer_(NULL) { 

		write_kbuffer_ = NULL;
		
		_LOG_TRACE("LibeventTcpSocketImpl() %p\n", this);
	}

	~LibeventTcpSocketImpl() {
		_LOG_TRACE("~LibeventTcpSocketImpl : %p\n", this);

		_deleteTimer();

		if(write_evbuffer_) {
			evbuffer_free(write_evbuffer_);
			write_evbuffer_ = NULL;
		}

		if(write_kbuffer_) {
			kbuffer_free(write_kbuffer_);
			write_kbuffer_ = NULL;
		}
		_close();
	}

	static void bufevent_cb(struct bufferevent *bev, short events, void *ptr) {
		LibeventTcpSocketImpl *SELF = (LibeventTcpSocketImpl *)ptr;
		CHECK_IOSERVICE_STOP_VOID_RETURN(SELF->ioService());

		if (events & BEV_EVENT_CONNECTED) {
			bufferevent_enable(bev, EV_READ|EV_WRITE);
			SELF->fire_onSocket_Connected();
		} else if (events & BEV_EVENT_ERROR) {
			SELF->fire_onSocket_Error(EVUTIL_SOCKET_ERROR());
		} else if (events & BEV_EVENT_EOF) {
			_LOG_INFO("got BEV_EVENT_EOF flag(0x%x), socket close = error %d\n", events, EVUTIL_SOCKET_ERROR());
			SELF->fire_onSocket_Close();
		}
	}

	static inline void event_cb(coconut_socket_t fd, short what, void *arg) {
		LibeventTcpSocketImpl *SELF = (LibeventTcpSocketImpl *)arg;
		CHECK_IOSERVICE_STOP_VOID_RETURN(SELF->ioService());

		if(what & EV_READ) {
			SELF->_onReadEvent(fd);
		} 
		if(what & EV_WRITE){
			SELF->_onWriteEvent(fd);
		}
	}
	
	static void write_cb(struct bufferevent *bev, void *ptr) {
		LibeventTcpSocketImpl *SELF = (LibeventTcpSocketImpl *)ptr;
		SELF->_onBufferEventWritten();
	}

	static void read_cb(struct bufferevent *bev, void *ptr) {
#define READ_MODE_3 
#if defined(READ_MODE_1)
		// BEST PERFOMANCE!!, BUT ONLY FOR ECHO SERVER..
		struct evbuffer *input = bufferevent_get_input(bev);
		struct evbuffer *output = bufferevent_get_output(bev);
		evbuffer_add_buffer(output, input);
#elif defined(READ_MODE_2)
		struct evbuffer *readBuffer = bufferevent_get_input(bev);
		size_t buffer_len = evbuffer_get_length(readBuffer);
		const void *data = evbuffer_pullup(readBuffer, buffer_len);

		//hexdump((const unsigned char*)data, buffer_len, stdout);

		LibeventTcpSocketImpl *SELF = (LibeventTcpSocketImpl *)ptr;
		CHECK_IOSERVICE_STOP_VOID_RETURN(SELF->ioService());
		SELF->fire_onSocket_Read(data, buffer_len);

		evbuffer_drain(readBuffer, buffer_len);
#elif defined(READ_MODE_3)
		LibeventTcpSocketImpl *SELF = (LibeventTcpSocketImpl *)ptr;
		CHECK_IOSERVICE_STOP_VOID_RETURN(SELF->ioService());
		SELF->fire_onSocket_ReadEvent(bufferevent_getfd(bev));	
#else
		// WORST PERFOMANCE!!!
		LibeventTcpSocketImpl *SELF = (LibeventTcpSocketImpl *)ptr;
		CHECK_IOSERVICE_STOP_VOID_RETURN(SELF->ioService());
		struct evbuffer *readBuffer = bufferevent_get_input(bev);
		size_t buffer_len = evbuffer_get_length(readBuffer);
		char *buffer = new char[buffer_len];
		evbuffer_remove(readBuffer, buffer, buffer_len);
		SELF->fire_onSocket_Read((const void *)buffer, buffer_len);
		delete [] buffer;
#endif
	}

	static void timer_cb(coconut_socket_t fd, short what, void *arg) {
		LibeventTcpSocketImpl *SELF  = (LibeventTcpSocketImpl *)arg;
		CHECK_IOSERVICE_STOP_VOID_RETURN(SELF->ioService());
		SELF->fire_onSocket_Error(COOKIE_ETIMEDOUT);
	}

public:
	boost::shared_ptr<IOService> ioService() {
		return owner_->ioService();
	}

	coconut_socket_t socketFD() {
		if(bev_)
			return bufferevent_getfd(bev_);
		if(ev_read_)
			return event_get_fd(ev_read_);
		return COOKIE_INVALID_SOCKET;	 // TODO rename!!!
	}

	void _makeTimer() {
		if(NULL == conn_timer_) {
			conn_timer_ = new Timer(owner_->ioService());
			conn_timer_->setEventHandler(this);
		}
	}

	void _deleteTimer() {
		if(conn_timer_) {
			delete conn_timer_;
			conn_timer_ = NULL;
		}
	}
	void _throwExceptionIfNotInitState() {
		if(BaseSocket::Disconnected != owner_->state())
			throw IllegalStateException("Already starting connection");
		if(ev_read_ || ev_write_ || bev_)
			throw IllegalStateException("Already starting connection");
	}

	void attachSocketHandle(coconut_socket_t fd, bool doInstallFlag) {
		ScopedIOServiceLock(owner_->ioService());

		_throwExceptionIfNotInitState();

#if defined(WIN32)	// for IOCP support
		_createBufferEvent(fd);
#else
		_createEvent(fd);
#endif
		owner_->setState(BaseSocket::Connected);

		if(doInstallFlag) {
			install();
		} else {
			// Must call install() later!
		}
	}

	void connect(const char *host, int port, int timeout) {
		ScopedIOServiceLock(owner_->ioService());

		host_ = host;
		port_ = port;
		timeout_ = timeout;
		path_.clear();

		connect();
	}

	void connectUnix(const char *path, int timeout) {
		ScopedIOServiceLock(owner_->ioService());
		path_ = path;
		timeout_ = timeout;
		
		connect();
	}

	void connect() {
		ScopedIOServiceLock(owner_->ioService());

		_throwExceptionIfNotInitState();

		if(timeout_ > 0) {
			_makeTimer();
			conn_timer_->setTimer(TIMERID_CONNECT, timeout_ * 1000, false);
		}

		errorDetected_ = false;
		owner_->setState(BaseSocket::Connecting);
		_close();

		_connect();

		install();
	}

	void _connect() {
		if(path_.size() > 0) {
			_connectUnix();
		} else {
			_connectTcp();
		}
	}

	void _connectTcp() {
		_LOG_DEBUG("TCP CONNECT START: host = %s, port = %d\n", host_.c_str(), port_);

		_createBufferEvent(-1);

		dnsbase_ = evdns_base_new((struct event_base *)owner_->ioService()->coreHandle(), 1);
		int ret = bufferevent_socket_connect_hostname(bev_, dnsbase_, AF_UNSPEC, host_.c_str(), port_);

		if(0 != ret) {
			_close();
			throw SocketException("Error starting connection");
		}
	}

	void _connectUnix() {
#if ! defined(WIN32)
		_LOG_DEBUG("UNIX CONNECT START: path = %s\n", path_.c_str());

		coconut_socket_t sock;
		if((sock = ::socket(AF_LOCAL, SOCK_STREAM, 0)) < 0)
			throw SocketException("Error creating unix domain socket");

		_createEvent(sock);

		struct sockaddr_un sun;
		memset(&sun, 0, sizeof(sun));
		sun.sun_family = AF_LOCAL;
		strcpy(sun.sun_path, path_.c_str());

		evutil_make_socket_nonblocking(sock);

		if(::connect(sock, (struct sockaddr *)&sun, sizeof(sun)) < 0) {
			_close();
			throw SocketException("Error starting connection");
		}
#else
		assert(false && "Windows not support Unix Domain Socket");
#endif
	}

	void _createEvent(coconut_socket_t fd) {
		if(NULL == ev_read_)
			ev_read_ = event_new((struct event_base *)owner_->ioService()->coreHandle(), fd, EV_READ|EV_PERSIST, event_cb, this);
		if(NULL == ev_write_)
			ev_write_ = event_new((struct event_base *)owner_->ioService()->coreHandle(), fd, EV_WRITE|EV_PERSIST, event_cb, this);

		if(NULL == write_evbuffer_) 
			write_evbuffer_ = evbuffer_new();
		if(NULL == write_kbuffer_) 
			write_kbuffer_ = kbuffer_new();

		owner_->fire_onSocket_Initialized();
	}

	void _createBufferEvent(coconut_socket_t fd) {
		if(NULL == bev_) {
			bev_ = bufferevent_socket_new((struct event_base *)owner_->ioService()->coreHandle(), fd, BEV_OPT_CLOSE_ON_FREE);
			bufferevent_enable(bev_, EV_READ|EV_WRITE);
		}
	
		if(NULL == write_evbuffer_) 
			write_evbuffer_ = evbuffer_new();
		if(NULL == write_kbuffer_) 
			write_kbuffer_ = kbuffer_new();

		owner_->fire_onSocket_Initialized();
	}

	void install() {
		ScopedIOServiceLock(owner_->ioService());

		if(bev_) {
			bufferevent_setcb(bev_, read_cb, write_cb, bufevent_cb, this);

			struct evbuffer *readBuffer = bufferevent_get_input(bev_);
			if(readBuffer) {
				size_t buffer_len = evbuffer_get_length(readBuffer);
				if(buffer_len > 0) {
					read_cb(bev_, this);
				}
			}
		}
		if(ev_read_) {
			event_add(ev_read_, NULL);
		}
		if(ev_write_) {
			event_add(ev_write_, NULL);
		}
	}

	const void* _readBufferEvent(struct evbuffer *buffer, size_t &size) {
		size_t buffer_len = evbuffer_get_length(buffer);
		if(buffer_len <= 0) {
			errno = EAGAIN; // TODO multi platform EAGAIN SETTING.. but this code may work on Win32 by CRT..
			return NULL;
		}
		if(buffer_len < size)
			size = buffer_len;
		const void *pulled_data = evbuffer_pullup(buffer, size);
		//logger::hexdump((const unsigned char*)pulled_data, size, stdout);
		return pulled_data;
	}

	void checkResponseSocket(int res) {
		if (res == -1) {
			int err = evutil_socket_geterror(socketFD());
			if (!SOCKET_ERR_RW_RETRIABLE(err)) {
				fire_onSocket_Error(err);
			}
		} else if (res == 0) {
			/* eof case */
			_LOG_DEBUG("checkResponseSocket res is 0, socket close = error %d\n", EVUTIL_SOCKET_ERROR());
			fire_onSocket_Close();
		}
	}

	int read(std::string &data, size_t size) {
		if(bev_) {
			struct evbuffer *readBuffer = bufferevent_get_input(bev_);
			const char *pulled_data = (const char *)_readBufferEvent(readBuffer, size);
			if(pulled_data)
				data.assign(pulled_data, size);
			else
				return -1;
			evbuffer_drain(readBuffer, size);
			return size;
		} else {
			char *p = (char *)malloc(size);
			if(NULL == p) 
				return -1;
#if defined(WIN32)
			int res = ::recv(socketFD(), (char *)p, size, 0);
#else
			int res = ::read(socketFD(), p, size);
#endif
			if(res > 0)
				data.assign(p, res);
			else
				checkResponseSocket(res);
			free(p);
			return res;
		}
	}

	int read(void *data, size_t size) {
		if(bev_) {
			struct evbuffer *readBuffer = bufferevent_get_input(bev_);
			const char *pulled_data = (const char *)_readBufferEvent(readBuffer, size);
			if(pulled_data)
				memcpy(data, pulled_data, size);
			else 
				return -1;
			evbuffer_drain(readBuffer, size);
			return size;
		} else {
#if defined(WIN32)
			int res = ::recv(socketFD(), (char *)data, size, 0);
#else
			int res = ::read(socketFD(), data, size);
#endif
			if(res <= 0)
				checkResponseSocket(res);
			return res;
		}
	}

	int write(const void *data, size_t size) {
		ScopedIOServiceLock(owner_->ioService());

		int ret = -1;
		if(size <= 0)
			return 0;

		if(BaseSocket::Connected != owner_->state()) {
			if(pendingWriteSupported_) {
#if defined(_KBUFFER_)
				ret = kbuffer_add(write_kbuffer_, data, size);
#else
				// pending data collected for reconnect.
				ret = evbuffer_add(write_evbuffer_, data, size);
				if(ret == 0) {
					ret = size;
				}
#endif
			} else {
				return 0;
			}
		}

		if(bev_) {
			// DO NOT USE write_evbuffer_, just use bufferevent
			struct evbuffer *output = bufferevent_get_output(bev_);
			ret = evbuffer_add(output, data, size);
			if(ret == 0) {
				//bufferevent_enable(bev_, EV_WRITE);
				ret = size;
			}
		} else if(ev_write_) {
#if defined(_KBUFFER_)
			ret = kbuffer_add(write_kbuffer_, data, size);
#else
			ret = evbuffer_add(write_evbuffer_, data, size);
			if(ret == 0) {
				ret = size;
			}
#endif
			event_add(ev_write_, NULL);
		}

		return ret;
	}

	const BaseAddress * peerAddress() {
		// TODO : if unix domain socket ??
		struct sockaddr_in sin;
		socklen_t sa_len = sizeof(sin);
		getpeername(socketFD(), (struct sockaddr *)&sin, &sa_len);
		peerAddress_.setSocketAddress(&sin);
		return &peerAddress_;
	}

	const BaseAddress * sockAddress() {
		// TODO : if unix domain socket ??
		struct sockaddr_in sin;
		socklen_t sa_len = sizeof(sin);
		getsockname(socketFD(), (struct sockaddr *)&sin, &sa_len);
		sockAddress_.setSocketAddress(&sin);
		return &sockAddress_;
	}

	void closeAfterAllSent() {
		expired_ = true;
		if(bev_) {
			bufferevent_enable(bev_, EV_WRITE);
			
			// following function is checking output buffer is empty. if yes, close
			_checkOutpuBufferAndClose();
		} else {
			event_del(ev_read_);
			event_add(ev_write_, NULL);
		}

		// wait for sending all buffer.
	}

	void close() {
		_close();
		fire_onSocket_Close();
	}

	void _close() {
		ScopedIOServiceLock(owner_->ioService());

		_LOG_DEBUG("close() this=%p threadid=%p >> %d %p %p %p\n", 
				this, owner_->ioService()->nativeThreadHandle(), socketFD(),
				bev_, ev_read_, ev_write_);

		if(dnsbase_) {
			evdns_base_free(dnsbase_, 0);
			dnsbase_ = NULL;
		}
		if(bev_) {
			bufferevent_free(bev_);
			bev_ = NULL;
		}
		if(ev_read_) {
			event_free(ev_read_);
			ev_read_ = NULL;
		}
		if(ev_write_) {
			coconut_socket_t fd = event_get_fd(ev_write_);
			evutil_closesocket(fd);

			event_free(ev_write_);
			ev_write_ = NULL;
		}
	}

	void _checkOutpuBufferAndClose() {
		if(expired_) {
			if(bev_) {
				struct evbuffer *output = bufferevent_get_output(bev_);
				if(evbuffer_get_length(output) <= 0) {
					_LOG_DEBUG("(bufferevent) this socket is expired! socket closed.. : %d", socketFD());
					close();	// directly close!
					return;
				}
			}
		}
	}

	void _onBufferEventWritten() {
		_checkOutpuBufferAndClose();
	}

	void _onReadEvent(coconut_socket_t fd) {
		fire_onSocket_ReadEvent(fd);	
	}

	void _onWriteEvent(coconut_socket_t fd) {
		ScopedIOServiceLock(owner_->ioService());
		if(BaseSocket::Connecting != owner_->state()) {
#if defined(_KBUFFER_)
#define _AE_CLOSE 1
#define _AE_ERROR 2
			int size;
			int destroy = 0;
			do {
				if(kbuffer_get_size(write_kbuffer_) <= 0) {
					event_del(ev_write_);
					if(expired_) {
						_LOG_DEBUG("(event) this socket is expired! socket closed.. : %d", socketFD());
						close();
					}
					break;
				}
				const void *data = kbuffer_get_contiguous_data(write_kbuffer_, &size);
#if defined(WIN32)
				int nwrite = ::send(fd, (const char *)data, size, 0);
#else
				int nwrite = ::write(fd, data, size);
#endif
				if(nwrite > 0) {
					kbuffer_drain(write_kbuffer_, nwrite);
				} else if(nwrite == 0) {
					destroy = _AE_CLOSE;
					break;
				} else if(nwrite < 0) { 
					if(errno == EAGAIN) {
						nwrite = 0;
					} else {
						destroy = _AE_ERROR;
					}
					break;
				}
			} while(false);

			if(destroy) {
				_LOG_FATAL("write error  size = %d, reason = %d, fd = %d, errno = %d", size, destroy, socketFD(), EVUTIL_SOCKET_ERROR())
				close();
			}
#else
			evbuffer_write(write_evbuffer_, fd);	
			if(evbuffer_get_length(write_evbuffer_) <= 0) {
				event_del(ev_write_);
			}
#endif
		} else {
			event_del(ev_write_);
			fire_onSocket_Connected();
		}
	}

	void onTimer_Timer(int id) {
		fire_onSocket_Error(COOKIE_ETIMEDOUT);
	}

private:	// fire event callback
	inline void fire_onSocket_ReadEvent(int fd) {
		owner_->fire_onSocket_ReadEvent(fd);
	}

	/*
	void fire_onSocket_Read(const void *data, int size) {
		owner_->eventHandler()->onSocket_Read(data, size);
	}
	*/

	void fire_onSocket_Close() {
		_LOG_DEBUG("fire_onSocket_Close : this = %p, fd = %d, handler = %p, errno = %d", 
				this, socketFD(), owner_->eventHandler(), EVUTIL_SOCKET_ERROR());
		_deleteTimer();
		owner_->setState(BaseSocket::Disconnected);
		_close();
		owner_->fire_onSocket_Close();
	}

	void fire_onSocket_Error(int error) {
		_LOG_DEBUG("fire_onSocket_Error : this = %p, fd = %d, error = %d, %s, handler = %p\n", 
					this, socketFD(), EVUTIL_SOCKET_ERROR(), evutil_socket_error_to_string(error), owner_->eventHandler());

		errorDetected_ = true;
		owner_->setLastErrorString(evutil_socket_error_to_string(error));

		if(BaseSocket::Connecting == owner_->state()) {
			int err = bufferevent_socket_get_dns_error(bev_);
			if(err) {
				owner_->setLastErrorString(evutil_gai_strerror(err));
			}
		}
		owner_->setState(BaseSocket::Disconnected);
		_close();
		owner_->fire_onSocket_Error(error, owner_->lastErrorString());
	}

	void fire_onSocket_Connected() {
		if(errorDetected_) {
			return;
		}
		owner_->setState(BaseSocket::Connected);

		if(pendingWriteSupported_) {
			ScopedIOServiceLock(owner_->ioService());

#if defined(_KBUFFER_)
			if(write_kbuffer_ && kbuffer_get_size(write_kbuffer_) > 0) {
				if(ev_write_)
					event_add(ev_write_, NULL);
			}
#else
			if(write_evbuffer_ && evbuffer_get_length(write_evbuffer_) > 0) {
				if(bev_) {
					// send pending data
					struct evbuffer *output = bufferevent_get_output(bev_);
					evbuffer_add_buffer(output, write_evbuffer_);
				}
			}
#endif
		}

		_deleteTimer();
		owner_->fire_onSocket_Connected();
	}

private:
	TcpSocket *owner_;
	volatile bool expired_;
	volatile bool errorDetected_;
	volatile bool pendingWriteSupported_;
	struct bufferevent *bev_;	
	struct event *ev_read_;
	struct event *ev_write_;
	std::string host_;
	int port_;
	std::string path_;
	int timeout_;
	struct evbuffer *write_evbuffer_;
	kbuffer *write_kbuffer_;
	struct evdns_base *dnsbase_;
	Timer *conn_timer_;

	IPv4Address peerAddress_;
	IPv4Address sockAddress_;
};

}

