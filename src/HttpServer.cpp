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

#include "CoconutLib.h"
#include "HttpServer.h"
#include "IOService.h"
#include "Exception.h"
#include "InternalLogger.h"
#include "HttpServerImpl.h"
#include "HttpRequest.h"
#include "BaseIOSystemFactory.h"

namespace coconut {

HttpServer::HttpServer(boost::shared_ptr<IOService> ioService, int port, boost::shared_ptr<HttpServer::EventHandler> handler) : timerObj_(NULL) {
	LOG_TRACE("HttpServer() : this = %p", this);
	impl_ = BaseIOSystemFactory::instance()->createHttpServerImpl();
	impl_->initialize(this, ioService, port);
	handler_ = handler;
}

HttpServer::~HttpServer() {
	LOG_TRACE("~HttpServer() : this = %p", this);
	_removeTimer();
}

boost::shared_ptr<IOService> HttpServer::ioService() {
	return impl_->ioService();
}

boost::shared_ptr<HttpServer::EventHandler> HttpServer::eventHandler() {
	return handler_;
}

void HttpServer::listen() {
	impl_->listen();
}

void HttpServer::fire_onHttpServer_DestroyRequest(boost::shared_ptr<HttpRequest> request) {
	LOG_DEBUG("fire_onHttpServer_DestroyRequest called : this = %p", this);
	if(handler_)
		handler_->onHttpServer_DestroyRequest(request);

#if defined(HAVE_LIBEVENT_GUNOODADDY_FIX)
	SetHttpRequests_t::iterator it = requests_.find(request);
	if(it != requests_.end()) {
		requests_.erase(it);
	}
#endif
}

void HttpServer::fire_onHttpServer_DocumentRequest(boost::shared_ptr<HttpRequest> request) {
	LOG_DEBUG("fire_onHttpServer_DocumentRequest called : this = %p", this);
	if(handler_)
		handler_->onHttpServer_DocumentRequest(request);
#if defined(HAVE_LIBEVENT_GUNOODADDY_FIX)
	requests_.insert(request);
#endif
}

void HttpServer::fire_onHttpServer_Initialized() {
	if(handler_)
		handler_->onHttpServer_Initialized();
}

void HttpServer::setTimer(unsigned short id, unsigned int msec, bool repeat) {
	ScopedIOServiceLock(ioService());
	_makeTimer();
	timerObj_->setTimer(id, msec, repeat);
}

void HttpServer::killTimer(unsigned short id) {
	ScopedIOServiceLock(ioService());
	_makeTimer();
	timerObj_->killTimer(id);
}

void HttpServer::_makeTimer() {
	if(NULL == timerObj_) {
		timerObj_ = Timer::make();
		timerObj_->initialize(ioService());
		timerObj_->setEventHandler(this);
	}
}

void HttpServer::onTimer_Timer(int id) {
	if(!(id & INTERNAL_TIMER_BIT)) {
		handler_->onHttpServer_Timer(id);
	}
}

void HttpServer::_removeTimer() {
	Timer::destroy(timerObj_);
}

}
