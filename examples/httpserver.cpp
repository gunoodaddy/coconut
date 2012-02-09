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
#include <event2/http.h>
#include <event2/buffer.h>

class HttpServerHandler : public coconut::HttpServer::EventHandler {
	virtual void onHttpServer_DocumentRequest(coconut::HttpServer *server, boost::shared_ptr<coconut::HttpRequest> request) { 

		// directly using libevent-http api
		{
			struct evhttp_request *req = (struct evhttp_request*)request->nativeHandle();
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
		coconut::HttpServer server(ioServiceContainer.ioServiceByRoundRobin(), port);
		server.setEventHandler(handler.get());
		server.start();

		LOG_INFO("httpserver started..");
		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		LOG_FATAL("Exception emitted : %s\n", e.what());
	}
	return 0;
}
