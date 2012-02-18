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
#include "HttpRequest.h"
#include "HttpRequestImpl.h"
#include "InternalLogger.h"
#include "BaseIOSystemFactory.h"
#include "HttpServer.h"

namespace coconut {

HttpRequest::HttpRequest(HttpServer *server, coconut_http_request_handle_t req) : serverInst_(server), native_handle_(req) { 
	_LOG_TRACE("HttpRequest() : this = %p", this);
	impl_ = BaseIOSystemFactory::instance()->createHttpRequestImpl();
	impl_->initialize(this);
}

HttpRequest::~HttpRequest() {
	_LOG_TRACE("~HttpRequest() : this = %p", this);
}

HttpServer *HttpRequest::server() {
	return serverInst_;
}

bool HttpRequest::isValidRequest() {
	return impl_->isValidRequest();
}

const char * HttpRequest::uri() {
	return impl_->uri();
}

const char * HttpRequest::path() {
	return impl_->path();
}

HttpMethodType HttpRequest::methodType() {
	return impl_->methodType();
}

const char * HttpRequest::findHeader(const char *key) {
	return impl_->findHeader(key);
}

const char * HttpRequest::findParameter(const char *key) {
	return impl_->findParameter(key);
}

size_t HttpRequest::parameterCountOf(const char *key) {
	return impl_->parameterCountOf(key);
}

const char * HttpRequest::findParameterOf(const char *key, size_t index) {
	return impl_->findParameterOf(key, index);
}

const std::string & HttpRequest::requestBody() {
	return impl_->requestBody();
}

bool HttpRequest::sendReplyString(int code, const char *reason, const std::string &str) {
	return impl_->sendReplyData(code, reason, str.c_str(), str.size());
}

bool HttpRequest::sendReplyData(int code, const char *reason, const char* data, size_t size) {
	return impl_->sendReplyData(code, reason, data, size);
}

void HttpRequest::dumpRequest(FILE *fp) {
	impl_->dumpRequest(fp);
}

}

