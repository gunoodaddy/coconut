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

#include "BaseIOSystemFactory.h"
#include "LibeventConnectionListenerImpl.h"
#include "LibeventDNSResolverImpl.h"
#include "LibeventTimerImpl.h"
#include "LibeventDeferredCallerImpl.h"
#include "LibeventTcpSocketImpl.h"
#include "LibeventUdpSocketImpl.h"
#include "LibeventIOServiceImpl.h"
#include "LibeventHttpRequestImpl.h"
#include "LibeventHttpServerImpl.h"
#include "LibeventHttpClientImpl.h"

namespace coconut {

class LibeventSystemFactory : public BaseIOSystemFactory {
public:
	virtual ~LibeventSystemFactory() { }

	boost::shared_ptr<ConnectionListenerImpl> createConnectionListenerImpl() {
		boost::shared_ptr<LibeventConnectionListenerImpl> p = LibeventConnectionListenerImpl::makeSharedPtr();
		return p;
	}

	boost::shared_ptr<DNSResolverImpl> createDNSResolverImpl() {
		boost::shared_ptr<LibeventDNSResolverImpl> p = LibeventDNSResolverImpl::makeSharedPtr();
		return p;
	}

	boost::shared_ptr<TimerImpl> createTimerImpl() {
		boost::shared_ptr<LibeventTimerImpl> p = LibeventTimerImpl::makeSharedPtr();
		return p;
	}

	boost::shared_ptr<DeferredCallerImpl> createDeferredCallerImpl() { 
		return LibeventDeferredCallerImpl::makeSharedPtr();
	}

	boost::shared_ptr<TcpSocketImpl> createTcpSocketImpl() {
		boost::shared_ptr<LibeventTcpSocketImpl> p = LibeventTcpSocketImpl::makeSharedPtr();
		return p;
	}

	boost::shared_ptr<UdpSocketImpl> createUdpSocketImpl() {
		boost::shared_ptr<LibeventUdpSocketImpl> p = LibeventUdpSocketImpl::makeSharedPtr();
		return p;
	}

	boost::shared_ptr<IOServiceImpl> createIOServiceImpl() {
		boost::shared_ptr<LibeventIOServiceImpl> p = LibeventIOServiceImpl::makeSharedPtr();
		return p;
	}

	boost::shared_ptr<HttpServerImpl> createHttpServerImpl() {
		boost::shared_ptr<LibeventHttpServerImpl> p = LibeventHttpServerImpl::makeSharedPtr();
		return p;
	}

	boost::shared_ptr<HttpRequestImpl> createHttpRequestImpl() {
		boost::shared_ptr<LibeventHttpRequestImpl> p = LibeventHttpRequestImpl::makeSharedPtr();
		return p;
	}

	boost::shared_ptr<HttpClientImpl> createHttpClientImpl() {
		boost::shared_ptr<LibeventHttpClientImpl> p = LibeventHttpClientImpl::makeSharedPtr();
		return p;
	}
};

}
