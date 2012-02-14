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
class HttpRequest;
class HttpRequestImpl;
class IOServiceImpl;
class BaseIOServiceContainer;

class BaseIOSystemFactory {
public:
	virtual ~BaseIOSystemFactory() { }

	virtual boost::shared_ptr<ConnectionListenerImpl> createConnectionListenerImpl() = 0;

	virtual boost::shared_ptr<DNSResolverImpl> createDNSResolverImpl() = 0;
	
	virtual boost::shared_ptr<TimerImpl> createTimerImpl() = 0;
	
	virtual boost::shared_ptr<DeferredCallerImpl> createDeferredCallerImpl() = 0;

	virtual boost::shared_ptr<TcpSocketImpl> createTcpSocketImpl() = 0;

	virtual boost::shared_ptr<UdpSocketImpl> createUdpSocketImpl() = 0;
	
	virtual boost::shared_ptr<IOServiceImpl> createIOServiceImpl() = 0;

	virtual boost::shared_ptr<HttpRequestImpl> createHttpRequestImpl() = 0;

	virtual boost::shared_ptr<HttpServerImpl> createHttpServerImpl() = 0;

	virtual boost::shared_ptr<HttpClientImpl> createHttpClientImpl() = 0;

public:
	static boost::shared_ptr<BaseIOSystemFactory> instance() {
		return factory_;
	}
	static void setInstance(boost::shared_ptr<BaseIOSystemFactory> factory) {
		factory_ = factory;
	}

private:
	static boost::shared_ptr<BaseIOSystemFactory> factory_;
};

}
