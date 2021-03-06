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

#ifdef HAVE_LIBHIREDIS

#include "RedisResponse.h"

namespace coconut {

class IOService;
class RedisRequestImpl;

class COCONUT_API RedisRequest {
public:
	RedisRequest(boost::shared_ptr<IOService> ioService, const char *host, int port/*, int timeout*/);
	~RedisRequest();

	struct requestContext;
	typedef boost::function< void (const struct requestContext *, boost::shared_ptr<RedisResponse>) > ResponseHandler;

	struct requestContext {
		ticket_t ticket;
		std::string command;
		std::vector<std::string> args;
		ResponseHandler handler;
		int ttl;	// private
	};

public:
	boost::shared_ptr<IOService> ioService();

	void connect();
	void close();

	ticket_t command(const std::string &cmd, const std::string args, RedisRequest::ResponseHandler handler, int timeout/*sec*/= 0);
	ticket_t command(const std::vector<std::string> &args, ResponseHandler handler, int timeout/*sec*/= 0);
	void cancel(ticket_t ticket);

private:
	RedisRequestImpl *impl_;
};

}
#endif
