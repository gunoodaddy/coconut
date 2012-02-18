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

#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <set>
#endif
#include "Timer.h"

typedef boost::function< void () > httpURICallback_t;

struct evhttp_request;

namespace coconut {

class IOService;
class Timer;
class HttpRequest;
class HttpServerImpl;

class COCONUT_API HttpServer : public Timer::EventHandler {
public:
	class EventHandler
	{
	public:
		virtual ~EventHandler() { }
		virtual void onHttpServer_Initialized() { }
		virtual void onHttpServer_DocumentRequest(boost::shared_ptr<HttpRequest> request) { }
		virtual void onHttpServer_Timer(unsigned short id) { }
		virtual void onHttpServer_DestroyRequest(boost::shared_ptr<HttpRequest> request) { }
		// maybe another callback added..
		// if decided one callback count (like now), TODO replace boost::bind mechanism..
	};

	HttpServer(boost::shared_ptr<IOService> ioService, int port, boost::shared_ptr<EventHandler> handler);
	virtual ~HttpServer();
public:
	boost::shared_ptr<IOService> ioService();
	boost::shared_ptr<EventHandler> eventHandler();

	void listen();

	void setTimer(unsigned short id, unsigned int msec, bool repeat);
	void killTimer(unsigned short id);
	
public:
	void fire_onHttpServer_Initialized();
	void fire_onHttpServer_DestroyRequest(boost::shared_ptr<HttpRequest> request); 
	void fire_onHttpServer_DocumentRequest(boost::shared_ptr<HttpRequest> request); 

	// internal timer function
private:
	void _makeTimer();
	void _removeTimer();
	void onTimer_Timer(int id);

private:
	boost::shared_ptr<HttpServerImpl> impl_;
	boost::shared_ptr<HttpServer::EventHandler> handler_;
	Timer *timerObj_;
	typedef std::set<boost::shared_ptr<HttpRequest> > SetHttpRequests_t;
	SetHttpRequests_t requests_;
};

}
