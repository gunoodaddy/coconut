#include "Coconut.h"
#include "HttpServer.h"
#include "IOService.h"
#include "Exception.h"
#include "Logger.h"
#include <event2/dns.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/keyvalq_struct.h>
#include <event2/buffer.h>
#include <fcntl.h>
#include <map>
#include <vector>

namespace coconut {

class HttpServerImpl {
public:
	HttpServerImpl(HttpServer *owner, 
				   boost::shared_ptr<IOService> ioService,
				   int port)
		: owner_(owner)
		, ioService_(ioService)
		, http_(NULL)
		, handle_(NULL)
		, port_(port)
	{
		LOG_TRACE("HttpServerImpl() : %p", this);
	}

	~HttpServerImpl() {
		LOG_TRACE("~HttpServerImpl() : %p", this);

		if(http_) {
			evhttp_free(http_);	
			http_ = NULL;
			handle_ = NULL;
		}
	}

private:
	static void http_path_callback(struct evhttp_request *req, void *arg) {
		HttpServerImpl *SELF = (HttpServerImpl *)arg;
		SELF->fire_onDocumentCallback(req);
	}

	void fire_onDocumentCallback(struct evhttp_request *req) {
		ScopedMutexLock(lock_);

		HttpRequest request(req);
		handler_->onHttpServer_DocumentRequest(owner_, &request);
/*
		const char *uri = evhttp_request_get_uri(req);
		mapcallback_t::iterator it = mapCallback_.find(uri);
		if(it != mapCallback_.end()) {
			it->second();
		} else {
    		evhttp_send_error(req, 404, "Document was not found");
		}
*/
	}

public:
	boost::shared_ptr<IOService> ioService() {
		return ioService_;
	}

	void setEventHandler(HttpServer::EventHandler *handler) {
		ScopedMutexLock(lock_);
		handler_ = handler;
	}

	HttpServer::EventHandler * eventHandler() {
		return handler_;
	}

	void start() {
		if(NULL == http_) {
			http_ = evhttp_new(ioService_->coreHandle());
		}

		evhttp_set_gencb(http_, http_path_callback, this);

		handle_ = evhttp_bind_socket_with_handle(http_, "0.0.0.0", port_);
		if (!handle_) {
			throw SocketException("couldn't bind to port for http server");
		}
	}

private:
	HttpServer *owner_;
	boost::shared_ptr<IOService> ioService_;
	HttpServer::EventHandler *handler_;
	struct evhttp *http_;
	struct evhttp_bound_socket *handle_; // this is not valid after \a http is freed.
	int port_;
	Mutex lock_;
//	typedef std::map<std::string, httpURICallback_t> mapcallback_t;
//	mapcallback_t mapCallback_;
};

/////////////////////////////////////////////////////////////////////////////////////////////

HttpServer::HttpServer(boost::shared_ptr<IOService> ioService, int port) {
	impl_ = new HttpServerImpl(this, ioService, port);
}

HttpServer::~HttpServer() {
	delete impl_;
}

boost::shared_ptr<IOService> HttpServer::ioService() {
	return impl_->ioService();
}

void HttpServer::setEventHandler(HttpServer::EventHandler *handler) {
	return impl_->setEventHandler(handler);
}

HttpServer::EventHandler* HttpServer::eventHandler() {
	return impl_->eventHandler();
}

void HttpServer::start() {
	impl_->start();
}

}
