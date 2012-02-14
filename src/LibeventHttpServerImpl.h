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

#include <event2/dns.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/keyvalq_struct.h>
#include <event2/buffer.h>
#include <fcntl.h>
#include <map>
#include <vector>
#include "HttpRequest.h"
#include "HttpServerImpl.h"
#include "BaseObjectAllocator.h"

namespace coconut {

class LibeventHttpServerImpl 
				: public HttpServerImpl 
				, public BaseObjectAllocator<LibeventHttpServerImpl>
{
public:
	LibeventHttpServerImpl()
		: owner_(NULL)
		, ioService_()
		, http_(NULL)
		, handle_(NULL)
		, port_(0)
	{
		_LOG_TRACE("LibeventHttpServerImpl() : %p", this);
	}

	LibeventHttpServerImpl(HttpServer *owner, 
				   boost::shared_ptr<IOService> ioService,
				   int port)
		: owner_(owner)
		, ioService_(ioService)
		, http_(NULL)
		, handle_(NULL)
		, port_(port)
	{
		_LOG_TRACE("LibeventHttpServerImpl() : %p", this);
	}

	~LibeventHttpServerImpl() {
		_LOG_TRACE("~LibeventHttpServerImpl() : %p", this);

		if(http_) {
			evhttp_free(http_);	
			http_ = NULL;
			handle_ = NULL;
		}
	}

	void initialize(HttpServer *owner, boost::shared_ptr<IOService> ioService, int port) {
		owner_ = owner;
		ioService_ = ioService;
		port_ = port;
	}

private:
	static void http_path_callback(struct evhttp_request *req, void *arg) {
		LibeventHttpServerImpl *SELF = (LibeventHttpServerImpl *)arg;
		SELF->fire_onDocumentCallback(req);
	}

	void fire_onDocumentCallback(struct evhttp_request *req) {
		ScopedMutexLock(lock_);

		boost::shared_ptr<HttpRequest> request 
				= boost::shared_ptr<HttpRequest>(new HttpRequest((coconut_http_request_handle_t)req));

		handler_->onHttpServer_DocumentRequest(owner_, request);
/*
		const char *uri = evhttp_request_get_uri(req);
		mapcallback_t::iterator it = mapCallback_.find(uri);
		if(it != mapCallback_.end()) {
			it->second();
		} else {
    		evhttp_send_error(req, 404, "Document was not found");
		}
*/
	}

public:
	boost::shared_ptr<IOService> ioService() {
		return ioService_;
	}

	void setEventHandler(HttpServer::EventHandler *handler) {
		ScopedMutexLock(lock_);
		handler_ = handler;
	}

	HttpServer::EventHandler * eventHandler() {
		return handler_;
	}

	void start() {
		if(NULL == http_) {
			http_ = evhttp_new((struct event_base *)ioService_->coreHandle());
		}

		evhttp_set_gencb(http_, http_path_callback, this);

		handle_ = evhttp_bind_socket_with_handle(http_, "0.0.0.0", port_);
		if (!handle_) {
			throw SocketException("couldn't bind to port for http server");
		}
		handler_->onHttpServer_Initialized(owner_);
	}

private:
	HttpServer *owner_;
	boost::shared_ptr<IOService> ioService_;
	HttpServer::EventHandler *handler_;
	struct evhttp *http_;
	struct evhttp_bound_socket *handle_; // this is not valid after \a http is freed.
	int port_;
	Mutex lock_;
//	typedef std::map<std::string, httpURICallback_t> mapcallback_t;
//	mapcallback_t mapCallback_;
};

}

