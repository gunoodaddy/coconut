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

//#define USE_PENDING_RESPONSE

class HttpServerHandler : public coconut::HttpServer::EventHandler {
	typedef std::list<boost::shared_ptr<coconut::HttpRequest> > ListRequest_t;	
	
	void sendResponse(boost::shared_ptr<coconut::HttpRequest> request) {
		static int tick_ = 0;
		char res[1024];
		sprintf(res, "<html>\n <head>\n"
				"<title>%s</title>\n"
				"</head>\n"
				"<body>\n"
				"<h1>hello world : hi~ [%s:%d]</h1>\n"
				"<ul>\n", request->uri(), request->uri(), tick_);
	
		LOG_DEBUG("onHttpServer_Timer emitted : sendReplyString for %s\n", request->uri());

		request->sendReplyString(200, "OK", res);
		tick_++;
	}

#ifdef USE_PENDING_RESPONSE
	virtual void onHttpServer_Timer(unsigned short id) {
		if(pendingRequests_.size() <= 0)
			return;

		boost::shared_ptr<coconut::HttpRequest> request = *pendingRequests_.begin();
		sendResponse(request);
		pendingRequests_.pop_front();
	}
#endif

	virtual void onHttpServer_DocumentRequest(boost::shared_ptr<coconut::HttpRequest> request) { 
		LOG_DEBUG("onHttpServer_DocumentRequest emitted.. uri = %s, path = %s", request->uri(), request->path());

		request->dumpRequest(stdout);
#ifdef USE_PENDING_RESPONSE
		pendingRequests_.push_back(request);
#else
		sendResponse(request);
#endif
	}

private:
	ListRequest_t pendingRequests_;
};


int main(int argc, char **argv) {
	if(argc < 2) {
		printf("usage : %s [port] [verbose:1,0]\n", argv[0]);
		return -1;
	}

	int port = atoi(argv[1]);
	if(argc > 2 && atoi(argv[2]) == 1) {
		coconut::logger::setLogLevel(coconut::logger::LEVEL_TRACE);
		coconut::setEnableDebugMode();
	}

	coconut::IOServiceContainer ioServiceContainer;
	ioServiceContainer.initialize();
	
	try {
		boost::shared_ptr<HttpServerHandler> handler(new HttpServerHandler);
		coconut::HttpServer server(ioServiceContainer.ioServiceByRoundRobin(), port, handler);
		server.listen();
#ifdef USE_PENDING_RESPONSE
		server.setTimer(0xFFFF, 5000, true);
#endif
		LOG_INFO("httpserver started..");
		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		LOG_FATAL("Exception emitted : %s\n", e.what());
	}
	return 0;
}

