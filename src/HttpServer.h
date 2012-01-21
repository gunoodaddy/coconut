#pragma once

#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#endif

typedef boost::function< void () > httpURICallback_t;

struct evhttp_request;

namespace coconut {

class IOService;
class HttpServerImpl;


class COCONUT_API HttpRequest {
public:
	HttpRequest(struct evhttp_request *req) : native_handle_(req) { }

	struct evhttp_request *nativeHandle() {
		return native_handle_;
	}

private:
	struct evhttp_request *native_handle_;
};


class COCONUT_API HttpServer {
public:
	HttpServer(boost::shared_ptr<IOService> ioService, int port);
	~HttpServer();

	class EventHandler
	{
	public:
		virtual ~EventHandler() { }
		virtual void onHttpServer_DocumentRequest(HttpServer *server, HttpRequest *request) { }
	};

public:
	boost::shared_ptr<IOService> ioService();

	void setEventHandler(EventHandler *handler);
	EventHandler * eventHandler();

	void start();

private:
	HttpServerImpl *impl_;
};

}
