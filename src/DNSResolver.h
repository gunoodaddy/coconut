#pragma once
#if defined(WIN32)
#include <ws2tcpip.h>
#else
#include <netdb.h>
#endif

namespace coconut {

class IOService;
class DNSResolverImpl;

class DNSResolver {
public:
	DNSResolver(boost::shared_ptr<IOService> ioService);
	~DNSResolver();

	class EventHandler {
		public:
			virtual ~EventHandler() { }
			virtual void onDnsResolveResult(int errcode, const char *host, struct addrinfo *addr, void *ptr) = 0;
	};

public:
	boost::shared_ptr<IOService> ioService() {
		return ioService_;
	}

	void cleanUp();
	bool resolve(const char *host, struct sockaddr_in *sin, EventHandler* handler, void *ptr);

private:
	DNSResolverImpl *impl_;
	boost::shared_ptr<IOService> ioService_;	
};

} // end of namespace coconut
