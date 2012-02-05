#pragma once

#include "IOSystemFactory.h"
#include "LibeventConnectionListenerImpl.h"
#include "LibeventDNSResolverImpl.h"
#include "LibeventTimerImpl.h"
#include "LibeventDeferredCallerImpl.h"
#include "LibeventTcpSocketImpl.h"
#include "LibeventUdpSocketImpl.h"
#include "LibeventIOServiceImpl.h"
#include "LibeventHttpServerImpl.h"
#include "LibeventHttpClientImpl.h"

namespace coconut {

class LibeventSystemFactory : public IOSystemFactory {
public:
	virtual ~LibeventSystemFactory() { }

	boost::shared_ptr<ConnectionListenerImpl> createConnectionListenerImpl(ConnectionListener *owner, int port) {
		return boost::shared_ptr<ConnectionListenerImpl>(new LibeventConnectionListenerImpl(owner, port));
	}

	boost::shared_ptr<ConnectionListenerImpl> createConnectionListenerImpl(ConnectionListener *owner, const char *path) {
		return boost::shared_ptr<ConnectionListenerImpl>(new LibeventConnectionListenerImpl(owner, path));
	}

	boost::shared_ptr<DNSResolverImpl> createDNSResolverImpl(boost::shared_ptr<IOService> ioService) {
		return boost::shared_ptr<DNSResolverImpl>(new LibeventDNSResolverImpl(ioService));
	}

	boost::shared_ptr<TimerImpl> createTimerImpl(Timer *owner) {
		return boost::shared_ptr<TimerImpl>(new LibeventTimerImpl(owner));
	}

	boost::shared_ptr<DeferredCallerImpl> createDeferredCallerImpl() { 
		return boost::shared_ptr<DeferredCallerImpl>(new LibeventDeferredCallerImpl());
	}

	boost::shared_ptr<DeferredCallerImpl> createDeferredCallerImpl(boost::shared_ptr<IOService> ioService) {
		return boost::shared_ptr<DeferredCallerImpl>(new LibeventDeferredCallerImpl(ioService));
	}

	boost::shared_ptr<TcpSocketImpl> createTcpSocketImpl(TcpSocket *owner) {
		return boost::shared_ptr<TcpSocketImpl>(new LibeventTcpSocketImpl(owner));
	}

	boost::shared_ptr<UdpSocketImpl> createUdpSocketImpl(UdpSocket *owner, int port) {
		return boost::shared_ptr<UdpSocketImpl>(new LibeventUdpSocketImpl(owner, port));
	}

	boost::shared_ptr<IOServiceImpl> createIOServiceImpl(int id, BaseIOServiceContainer *container, bool threadMode) {
		return boost::shared_ptr<IOServiceImpl>(new LibeventIOServiceImpl(id, container, threadMode));
	}

	boost::shared_ptr<HttpServerImpl> createHttpServerImpl(
															HttpServer *owner, 
															boost::shared_ptr<IOService> ioService, 
															int port) {
		return boost::shared_ptr<HttpServerImpl>(new LibeventHttpServerImpl(owner, ioService, port));
	}


	boost::shared_ptr<HttpClientImpl> createHttpClientImpl(
															HttpClient *owner, 
															boost::shared_ptr<IOService> ioService,
															HttpMethodType method = HTTP_GET,
															const char *uri = "",
															const HttpParameter *param = NULL,
															int timeout = 0) {
		return boost::shared_ptr<HttpClientImpl>(new LibeventHttpClientImpl(owner, ioService, method, uri, param, timeout));
	}


};

}
