#pragma once

#include "RedisResponse.h"

namespace coconut {

class IOService;
class RedisRequestImpl;

class COCONUT_API RedisRequest {
public:
	RedisRequest(boost::shared_ptr<IOService> ioService, const char *host, int port/*, int timeout*/);
	~RedisRequest();

	typedef boost::function< void (boost::shared_ptr<RedisResponse>) > ResponseHandler;

public:
	boost::shared_ptr<IOService> ioService();

	void connect();
	void close(bool callback = true);

	ticket_t command(const std::string &cmd, const std::vector<std::string> &args, ResponseHandler handler);
	void cancel(ticket_t ticket);

private:
	RedisRequestImpl *impl_;
};

}
