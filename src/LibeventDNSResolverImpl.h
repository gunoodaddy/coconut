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

#include <map>
#include <event2/dns.h>
#include "DNSResolver.h"
#include "DNSResolverImpl.h"
#include "BaseObjectAllocator.h"

#if defined(WIN32)
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif

namespace coconut { 

class LibeventDNSResolverImpl 
				: public DNSResolverImpl 
				, public BaseObjectAllocator<LibeventDNSResolverImpl>
{
public:
	LibeventDNSResolverImpl()
		: ioService_()
		, dnsbase_(NULL)
		, gotResult_(false) { }

	LibeventDNSResolverImpl(boost::shared_ptr<IOService> ioService) 
		: ioService_(ioService)
		, dnsbase_(NULL)
		, gotResult_(false) { }

	~LibeventDNSResolverImpl() {
		cleanUp();
	}

	void initialize(boost::shared_ptr<IOService> ioService) {
		ioService_ = ioService;
	}

private:
	struct dns_context_t {
		char *host_alloc;
		LibeventDNSResolverImpl *self;
		DNSResolver::EventHandler *handler;
		void *ptr;
	};

	static void callback(int errcode, struct addrinfo *addr, void *ptr) {
		struct dns_context_t *context = (struct dns_context_t*)ptr;
		context->self->fire_onDnsResolveResult(errcode, context->host_alloc, addr, context);

		_freeContext(context);
	}

	static void _freeContext(struct dns_context_t *context) {
		if(context) {
			if(context->host_alloc) 
				free(context->host_alloc);
			free(context);
		}
	}

public:
	void cleanUp()	{	
		// cancel all request & clean memory.
		if(dnsbase_) {
			evdns_base_free(dnsbase_, 0);
			dnsbase_ = NULL;
		}

		std::map<std::string, struct dns_context_t *>::iterator it = mapcontext_.begin();
		for(; it != mapcontext_.end(); it++) {
			_freeContext(it->second);
		}

		mapcontext_.clear();
	}

	bool resolve(const char *host, struct sockaddr_in *sin, DNSResolver::EventHandler* handler, void *ptr) {
		if(NULL == dnsbase_) {
			dnsbase_ = evdns_base_new((struct event_base *)ioService_->coreHandle(), 1);
		}

		std::map<std::string, struct dns_context_t *>::iterator it = mapcontext_.find(host);
		if(it != mapcontext_.end())
			return false;

		struct addrinfo hints;
		struct evdns_getaddrinfo_request *req;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = EVUTIL_AI_CANONNAME;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		struct dns_context_t *context;
		if (!(context = (struct dns_context_t *)malloc(sizeof(struct dns_context_t)))) {
			return false;
		}
		memset(context, 0, sizeof(struct dns_context_t));
		if (!(context->host_alloc = STRDUP(host))) {
			_freeContext(context);
			return false;
		}
		context->self = this;
		context->handler = handler;
		context->ptr = ptr;

		gotResult_ = false;
		req = evdns_getaddrinfo(
				dnsbase_, host, NULL /* no service name given */,
				&hints, callback, context);

		if (req == NULL) {
			if(gotResult_) {
				// request for this host returned immediately
				// called callback function already.. check it out!
				// DO NOT free context!
			} else {
				_freeContext(context);
			}
			return true;
		}

		mapcontext_.insert(std::map<std::string, struct dns_context_t *>::value_type(host, context));
		return true;
	}

private:
	void fire_onDnsResolveResult(int errcode, const char *host, struct addrinfo *addr, dns_context_t *context) {
		gotResult_ = true;
		context->handler->onDnsResolveResult(errcode, host, addr, context->ptr);

		std::map<std::string, struct dns_context_t *>::iterator it = mapcontext_.find(host);
		if(it != mapcontext_.end()) {
			mapcontext_.erase(it);
		}
	}

private:
	DNSResolver *owner_;
	boost::shared_ptr<IOService> ioService_;	
	struct evdns_base *dnsbase_;
	volatile bool gotResult_;

	std::map<std::string, struct dns_context_t *> mapcontext_;
};

}

