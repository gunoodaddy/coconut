#pragma once

//#include "HttpClient.h"

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
