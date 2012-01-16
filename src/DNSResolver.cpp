#include "Coconut.h"
#include "IOService.h"
#include "DNSResolver.h"
#include <map>
#include <event2/dns.h>

namespace coconut {

class DNSResolverImpl {
public:
	DNSResolverImpl(boost::shared_ptr<IOService> ioService) : ioService_(ioService), dnsbase_(NULL) { }
	~DNSResolverImpl() {
		cleanUp();
	}

private:
	struct dns_context_t {
		char *host_alloc;
		DNSResolverImpl *self;
		DNSResolver::EventHandler *handler;
		void *ptr;
	};

	static void callback(int errcode, struct addrinfo *addr, void *ptr) {
		struct dns_context_t *context = (struct dns_context_t*)ptr;
		context->self->fire_onDnsResolveResult(errcode, context->host_alloc, addr, context);

		free(context->host_alloc);
		free(context);
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
			free(it->second->host_alloc);
			free(it->second);
		}

		mapcontext_.clear();
	}

	bool resolve(const char *host, struct sockaddr_in *sin, DNSResolver::EventHandler* handler, void *ptr) {
		if(NULL == dnsbase_) {
			dnsbase_ = evdns_base_new(ioService_->base(), 1);
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
		if (!(context->host_alloc = strdup(host))) {
			free(context);
			return false;
		}
		context->host_alloc = strdup("localhost");
		context->self = this;
		context->handler = handler;
		context->ptr = ptr;

		req = evdns_getaddrinfo(
				dnsbase_, host, NULL /* no service name given */,
				&hints, callback, context);

		if (req == NULL) {
			// request for this host returned immediately
			// called callback function already.. check it out!
			return true;
		}

		mapcontext_.insert(std::map<std::string, struct dns_context_t *>::value_type(host, context));
		return true;
	}

private:
	void fire_onDnsResolveResult(int errcode, const char *host, struct addrinfo *addr, dns_context_t *context) {
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

	std::map<std::string, struct dns_context_t *> mapcontext_;
};

//-------------------------------------------------------------------------------------------------------

DNSResolver::DNSResolver(boost::shared_ptr<IOService> ioService) {
	impl_ = new DNSResolverImpl(ioService);
}

DNSResolver::~DNSResolver() {
	delete impl_;
}


void DNSResolver::cleanUp()	{	
	impl_->cleanUp();
}

bool DNSResolver::resolve(const char *host, struct sockaddr_in *sin, EventHandler* handler, void *ptr) {
	return impl_->resolve(host, sin, handler, ptr);
}

}
