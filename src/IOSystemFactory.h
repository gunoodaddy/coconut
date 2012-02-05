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

namespace coconut {

class IOService;
class ConnectionListener;
class ConnectionListenerImpl;
class DNSResolverImpl;
class Timer;
class TimerImpl;
class DeferredCallerImpl;
class TcpSocket;
class TcpSocketImpl;
class UdpSocket;
class UdpSocketImpl;
class HttpClientImpl;
class HttpParameter;
class HttpClient;
class HttpServer;
class HttpServerImpl;
class IOServiceImpl;
class BaseIOServiceContainer;

class IOSystemFactory {
public:
	virtual ~IOSystemFactory() { }

	virtual boost::shared_ptr<ConnectionListenerImpl> createConnectionListenerImpl(ConnectionListener *owner, int port) = 0;
	virtual boost::shared_ptr<ConnectionListenerImpl> createConnectionListenerImpl(ConnectionListener *owner, const char *path) = 0;

	virtual boost::shared_ptr<DNSResolverImpl> createDNSResolverImpl(boost::shared_ptr<IOService> ioService) = 0;
	
	virtual boost::shared_ptr<TimerImpl> createTimerImpl(Timer *owner) = 0;
	
	virtual boost::shared_ptr<DeferredCallerImpl> createDeferredCallerImpl() = 0;
	virtual boost::shared_ptr<DeferredCallerImpl> createDeferredCallerImpl(boost::shared_ptr<IOService> ioService) = 0;

	virtual boost::shared_ptr<TcpSocketImpl> createTcpSocketImpl(TcpSocket *owner) = 0;

	virtual boost::shared_ptr<UdpSocketImpl> createUdpSocketImpl(UdpSocket *owner, int port) = 0;
	
	virtual boost::shared_ptr<IOServiceImpl> createIOServiceImpl(int id, BaseIOServiceContainer *container, bool threadMode) = 0;

	virtual boost::shared_ptr<HttpServerImpl> createHttpServerImpl(
							HttpServer *owner, 
							boost::shared_ptr<IOService> ioService, 
							int port) = 0;

	virtual boost::shared_ptr<HttpClientImpl> createHttpClientImpl(
							HttpClient *owner, 
							boost::shared_ptr<IOService> ioService, 
							HttpMethodType method = HTTP_GET,
							const char *uri = "",
							const HttpParameter *param = NULL,
							int timeout = 0) = 0;

public:
	static boost::shared_ptr<IOSystemFactory> instance() {
		return factory_;
	}
	static void setInstance(boost::shared_ptr<IOSystemFactory> factory) {
		factory_ = factory;
	}

private:
	static boost::shared_ptr<IOSystemFactory> factory_;
};

}
