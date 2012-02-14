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
#include "HttpRequestImpl.h"
#include "BaseObjectAllocator.h"

namespace coconut {

class LibeventHttpRequestImpl 
				: public HttpRequestImpl 
				, public BaseObjectAllocator<LibeventHttpRequestImpl>
{
public:
	LibeventHttpRequestImpl()
		: owner_(NULL)
		, req_(NULL)
		, evuri_(NULL)
	{
		_LOG_TRACE("LibeventHttpRequestImpl() : %p", this);
	}

	LibeventHttpRequestImpl(HttpRequest *owner) 
		: owner_(owner)
		, req_(NULL)
		, evuri_(NULL)
	{
		req_ = (struct evhttp_request *)owner_->nativeHandle();
		_LOG_TRACE("LibeventHttpRequestImpl() : this = %p, req = %p", this, req_);
	}

	~LibeventHttpRequestImpl() {
		if(evuri_) {
			evhttp_uri_free(evuri_);
			evuri_ = NULL;
		}
		_LOG_TRACE("~LibeventHttpRequestImpl() : %p", this);
	}

	void initialize(HttpRequest *owner) {
		owner_ = owner;
		req_ = (struct evhttp_request *)owner_->nativeHandle();
	}

	const char *uri() {
		return evhttp_request_get_uri(req_);
	}

	const char *path() {
		if(NULL == evuri_) {
			evuri_ = evhttp_uri_parse_with_flags (uri(), 0);
		}
		return evhttp_uri_get_path(evuri_);
	}

	const char *findParameter(const char *key) {
		const char *theUri = uri();
		struct evkeyvalq headers;
		TAILQ_INIT(&headers);
		evhttp_parse_query(theUri, &headers);

		return evhttp_find_header(&headers, key);
	}

	void sendReplyData(int code, const char *reason, const char* data, size_t size) {
		struct evbuffer *evb = evbuffer_new();
		/*int ret =*/ evbuffer_add(evb, data, size);
		evhttp_send_reply(req_, code, reason, evb);
	}

private:
	HttpRequest *owner_;
	struct evhttp_request *req_;
	struct evhttp_uri* evuri_;
};

}

