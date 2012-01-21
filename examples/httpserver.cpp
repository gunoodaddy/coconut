#include "Coconut.h"
#include "IOServiceContainer.h"
#include "HttpServer.h"
#include "Exception.h"
#include "Logger.h"
#include <event2/http.h>
#include <event2/buffer.h>

class HttpServerHandler : public coconut::HttpServer::EventHandler {
	virtual void onHttpServer_DocumentRequest(coconut::HttpServer *server, coconut::HttpRequest *request) { 

		// directly using libevent-http api
		{
			struct evhttp_request *req = request->nativeHandle();
			const char *uri = evhttp_request_get_uri(req);
			LOG_INFO("onHttpServer_DocumentRequest emitted.. uri = %s", uri);

			struct evbuffer *evb = NULL;
			evb = evbuffer_new();
			evbuffer_add_printf(evb, 
					"<html>\n <head>\n"
					"<title>%s</title>\n"
					"</head>\n"
					"<body>\n"
					"<h1>hello workd : hi~ [%s]</h1>\n"
					"<ul>\n", uri, uri);
			evhttp_send_reply(req, 200, "OK", evb);
		}
	}
};


int main(int argc, char **argv) {
	if(argc < 2) {
		printf("usage : %s [port]\n", argv[0]);
		return -1;
	}

	int port = atoi(argv[1]);
	coconut::IOServiceContainer ioServiceContainer;
	ioServiceContainer.initialize();
	
	try {
		boost::shared_ptr<HttpServerHandler> handler(new HttpServerHandler);
		coconut::HttpServer server(ioServiceContainer.ioService(), port);
		server.setEventHandler(handler.get());
		server.start();

		LOG_INFO("httpserver started..");
		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		LOG_FATAL("Exception emitted : %s\n", e.what());
	}
	return 0;
}
