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

#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#endif

namespace coconut {

class HttpRequestImpl;
class HttpServer;

class COCONUT_API HttpRequest : public boost::enable_shared_from_this<HttpRequest> {
public:
	HttpRequest(HttpServer *server, coconut_http_request_handle_t req);
	~HttpRequest();

	coconut_http_request_handle_t nativeHandle() {
		return native_handle_;
	}

	boost::shared_ptr<HttpRequest> sharedMyself() {
		return shared_from_this();
	}

	HttpServer *server();
	bool isValidRequest();
	HttpMethodType methodType();
	const char * uri();
	const char * path();
	const char * findHeader(const char *key);
	const char * findParameter(const char *key);
	const char * findParameterOf(const char *key, size_t index);
	size_t parameterCountOf(const char *key);
	const std::string & requestBody();
	bool sendReplyString(int code, const char *reason, const std::string &str);
	bool sendReplyData(int code, const char *reason, const char* data, size_t size);
	void dumpRequest(FILE *fp);

private:
	HttpServer *serverInst_;
	coconut_http_request_handle_t native_handle_;
	boost::shared_ptr<HttpRequestImpl> impl_;
};

}
