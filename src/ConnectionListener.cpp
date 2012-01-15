#include "Coconut.h"
#if ! defined(WIN32)
#include <sys/un.h>
#endif
#include <errno.h>
#include <event2/util.h>
#include <event2/listener.h>
#include "IOService.h"
#include "ConnectionListener.h"
#include "Exception.h"
#include "Logger.h"

namespace coconut {

class ConnectionListenerImpl {
public:
	ConnectionListenerImpl(ConnectionListener *owner, int port) 
		: owner_(owner)
		, listener_(NULL)
		, path_("")
		, port_(port) { }

	ConnectionListenerImpl(ConnectionListener *owner, const char* path)
		: owner_(owner)
		, listener_(NULL)
		, path_(path)
		, port_(0) { }

	~ConnectionListenerImpl(void) {
		if(listener_)
			evconnlistener_free(listener_);
	}

private:
	static void accept_conn_cb(
					struct evconnlistener *listener,
					coconut_socket_t fd, struct sockaddr *address, int socklen,
					void *ctx) {
		ConnectionListenerImpl *SELF = (ConnectionListenerImpl *)ctx;
		SELF->fire_onConnectionListener_Accept(fd);
	}

	static void accept_error_cb(struct evconnlistener *listener, void *ctx) {
		int err = EVUTIL_SOCKET_ERROR();

		ConnectionListenerImpl *SELF = (ConnectionListenerImpl *)ctx;
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

		listener_ = evconnlistener_new_bind(owner_->ioService()->base(), accept_conn_cb, this,
				LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
				sinptr, addrlen);

		if(!listener_) {
			throw SocketException("Couldn't create listener");
		}
		evconnlistener_set_error_cb(listener_, accept_error_cb);
		LOG_DEBUG("Connection Listener started.. port : %d", port_);
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

//------------------------------------------------------------------------------------------------------

ConnectionListener::ConnectionListener(boost::shared_ptr<IOService> ioService, int port)
	: ioService_(ioService), handler_(NULL) {
	impl_ = new ConnectionListenerImpl(this, port);
}

ConnectionListener::ConnectionListener(boost::shared_ptr<IOService> ioService, const char* path)
	: ioService_(ioService), handler_(NULL) {
	impl_ = new ConnectionListenerImpl(this, path);
}

ConnectionListener::~ConnectionListener(void) {
	delete impl_;
}

void ConnectionListener::listen() {
	impl_->listen();
}

const char* ConnectionListener::listeningPath() {
	return impl_->listeningPath().c_str();
}

}
