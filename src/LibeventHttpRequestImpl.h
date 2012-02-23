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
#include "HttpServer.h"
#include "HttpRequest.h"
#include "HttpRequestImpl.h"
#include "BaseObjectAllocator.h"
#include "dep/MPFDParser/Parser.h"
#include "IPv4Address.h"

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
		, parseDone_(false)
	{
		_LOG_TRACE("LibeventHttpRequestImpl() : %p", this);
	}

	LibeventHttpRequestImpl(HttpRequest *owner) 
		: owner_(owner)
		, req_(NULL)
		, evuri_(NULL)
		, parseDone_(false)
	{
		req_ = (struct evhttp_request *)owner_->nativeHandle();
		_LOG_TRACE("LibeventHttpRequestImpl() : this = %p, req = %p", this, req_);
	}

	~LibeventHttpRequestImpl() {
		if(evuri_) {
			evhttp_uri_free(evuri_);
			evuri_ = NULL;
		}
		if(req_) {
			evhttp_request_free(req_);
		}
		_LOG_TRACE("~LibeventHttpRequestImpl() : %p", this);
	}

#ifdef HAVE_LIBEVENT_GUNOODADDY_FIX
	static void request_free_cb(struct evhttp_request *req, void *arg) {
		
		LibeventHttpRequestImpl *SELF = (LibeventHttpRequestImpl *)arg;
		SELF->fire_onHttpServer_DestroyRequest(req);
	}

	void fire_onHttpServer_DestroyRequest(struct evhttp_request *req) {
		owner_->server()->fire_onHttpServer_DestroyRequest(owner_->sharedMyself());
	}
#endif

	void initialize(HttpRequest *owner) {
		owner_ = owner;
		req_ = (struct evhttp_request *)owner_->nativeHandle();
		evhttp_request_own(req_);
#ifdef HAVE_LIBEVENT_GUNOODADDY_FIX
		evhttp_request_set_free_cb(req_, request_free_cb, this);
#endif
		struct evhttp_connection* conn =  evhttp_request_get_connection( req_ );
		if ( conn ) {
			char* ip = NULL;
			ev_uint16_t port = 0;
			evhttp_connection_get_peer(conn, &ip, &port );
			address_.setSocketAddress( ip, port );
		}
	}

	bool isValidRequest() {
		if( !req_ ) return false;
		_parseUri();
		if( NULL == evuri_ ) return false;

		return true;
	}

	HttpMethodType methodType() {
		if(!req_) { return HTTP_UNKNOWN; }
		if( evhttp_request_get_command(req_) & EVHTTP_REQ_GET) 
			return HTTP_GET;
		if( evhttp_request_get_command(req_) & EVHTTP_REQ_POST) 
			return HTTP_POST;
		return HTTP_UNKNOWN;
	}

	void dumpRequest(FILE *fp) {
		if(!req_) {
			if(fp) fprintf(fp, "http request is not initialized\n");
			return;
		}
		const char *cmdtype;
		struct evkeyvalq *headers;
		struct evkeyval *header;

		switch (evhttp_request_get_command(req_)) {
			case EVHTTP_REQ_GET: cmdtype = "GET"; break;
			case EVHTTP_REQ_POST: cmdtype = "POST"; break;
			case EVHTTP_REQ_HEAD: cmdtype = "HEAD"; break;
			case EVHTTP_REQ_PUT: cmdtype = "PUT"; break;
			case EVHTTP_REQ_DELETE: cmdtype = "DELETE"; break;
			case EVHTTP_REQ_OPTIONS: cmdtype = "OPTIONS"; break;
			case EVHTTP_REQ_TRACE: cmdtype = "TRACE"; break;
			case EVHTTP_REQ_CONNECT: cmdtype = "CONNECT"; break;
			case EVHTTP_REQ_PATCH: cmdtype = "PATCH"; break;
			default: cmdtype = "unknown"; break;
		}

		if(fp) fprintf(fp, "Received a %s request for %s\nHeaders:\n",
				cmdtype, evhttp_request_get_uri(req_));

		headers = evhttp_request_get_input_headers(req_);
		TAILQ_FOREACH(header, headers, next) {
			if(fp) fprintf(fp, "  %s: %s\n", header->key, header->value);
		}

		if(fp) fprintf(fp, "Input data: <<<\n");
		_storeRequestBody();
		if(fp) fprintf(fp, "%s\n", reqBody_.c_str());

		if(fp) fprintf(fp, "Input parameter: <<<\n");
		_parseParmeter();
		MapParameter_t::iterator it = parameters_.begin();
		for(; it != parameters_.end(); it++) {
			for(size_t j = 0; j < it->second.size(); j++) {
				if(fp) fprintf(fp, "%s => %s\n", it->first.c_str(), it->second[j].c_str());
			}
		}
	}

	void _storeRequestBody() {
		if(reqBody_.size() > 0) {
			// already stored..
			return;
		}
		struct evbuffer *buf;
		buf = evhttp_request_get_input_buffer(req_);
		while (evbuffer_get_length(buf)) {
			int n;
			char cbuf[128];
			n = evbuffer_remove(buf, cbuf, sizeof(buf)-1);
			if (n > 0)
				reqBody_.append(cbuf, n);
		}
	}

	void _parseUri() {
		if(NULL == evuri_) {
			evuri_ = evhttp_uri_parse_with_flags (uri(), 0);
			if(NULL == evuri_) {
				evuri_ = evhttp_uri_parse_with_flags (uri(), 1);
			}
		}
	}

	void _parseParmeter() {
		if(!req_) return;
		if(parseDone_) return;
		if(methodType() == HTTP_POST) {
			try {
				MPFD::Parser parser;
				parser.SetContentType(findHeader("Content-Type"));
				parser.AcceptSomeData(requestBody().c_str(), requestBody().size());
				const MPFD::Parser::ListParameter_t &reqMPFDs = parser.GetFields();
				MPFD::Parser::ListParameter_t::const_iterator it = reqMPFDs.begin();
				for(; it != reqMPFDs.end(); it++) {
					MPFD::Field *f = (*it);
					if(f->GetType() != MPFD::Field::TextType)
						continue;

					MapParameter_t::iterator itV = parameters_.find(f->key());
					if(itV == parameters_.end()) {
						// first insert
						std::vector<std::string> vec;
						vec.push_back(f->GetTextTypeContent());
						parameters_.insert(MapParameter_t::value_type(f->key(), vec));
					} else {
						itV->second.push_back(f->GetTextTypeContent());
					}
				}
			} catch (...) {
				// nothing.. skip error!
			}
		} else {
			const char *theUri = uri();
			struct evkeyvalq headers;
			struct evkeyval *header;
			TAILQ_INIT(&headers);
			int ret = evhttp_parse_query(theUri, &headers);
			if(ret != 0) {
				ret = evhttp_parse_query_str(theUri, &headers);
			}
			if(ret == 0) {
				bool bfirst = true;
				TAILQ_FOREACH(header, &headers, next) {
					std::string key = header->key;
					if(bfirst) {
						if(key.size() > 0) {
							size_t pos = key.find("?");
							if(pos != std::string::npos) {
								key.erase(0, pos+1);
							}
						}
						bfirst = false;
					}
					MapParameter_t::iterator itV = parameters_.find(key);
					if(itV == parameters_.end()) {
						// first insert
						std::vector<std::string> vec;
						vec.push_back(header->value);
						parameters_.insert(MapParameter_t::value_type(key, vec));
					} else {
						itV->second.push_back(header->value);
					}
				}
			}
		}
		parseDone_ = true;
	}

	const char *uri() {
		if(!req_) return "";
		return evhttp_request_get_uri(req_);
	}


	const char *path() {
		if(path_.size() != 0 ) return path_.c_str();
		if(!req_) return NULL;
		if(!evuri_) return NULL;

		const char* aPath = evhttp_uri_get_path(evuri_);
		path_ = aPath != NULL ? aPath : "";
		size_t idx = path_.rfind( '/' );
		if ( idx != path_.size() - 1 ) 
			return path_.c_str();

		path_.erase( idx );
		return path_.c_str();
	}

	const char *findHeader(const char *key) {
		if(!req_) return NULL;

		struct evkeyvalq *headers;
		struct evkeyval *header;
		headers = evhttp_request_get_input_headers(req_);
		for (header = headers->tqh_first; header; header = header->next.tqe_next) {
			if(strcmp(key, header->key) == 0)
				return header->value;
		}
		return NULL;
	}

	std::string _convertKeyArrayStyle(const char *key) {
		std::string theKey(key);
		size_t pos = theKey.find("[]");
		if(pos != theKey.size() - 2) {
			theKey += "[]";
		}
		return theKey;
	}

	size_t parameterCountOf(const char *key) {
		if(!req_) return 0;
		if(!key) return 0;
		if(!parseDone_) _parseParmeter();

		std::string theKey = _convertKeyArrayStyle(key);
		MapParameter_t::iterator it = parameters_.find(theKey);
		if(parameters_.end() != it) {
			return it->second.size();
		}
		return 0;
	}

	const char *findParameterOf(const char *key, size_t index) {
		if(!req_) return 0;
		if(!key) return 0;
		if(!parseDone_) _parseParmeter();

		std::string theKey = _convertKeyArrayStyle(key);
		MapParameter_t::iterator it = parameters_.find(theKey);
		if(parameters_.end() != it) {
			if(index < it->second.size())
				return it->second[index].c_str();
		}
		return NULL;
	}

	const char *findParameter(const char *key) {
		if(!req_) return 0;
		if(!key) return 0;
		if(!parseDone_) _parseParmeter();

		MapParameter_t::iterator it = parameters_.find(key);
		if(parameters_.end() != it) {
			if(it->second.size() > 0)
				return it->second[0].c_str();
		}
		return NULL;
	}

	const std::string & requestBody() {
		if(!req_) return reqBody_;

		_storeRequestBody();
		return reqBody_;
	}

	bool sendReplyData(int code, const char *reason, const char* data, size_t size) {
		if(!req_) return false;

		struct evbuffer *evb = evbuffer_new();
		/*int ret =*/ evbuffer_add(evb, data, size);
		evhttp_send_reply(req_, code, reason, evb);
		req_ = NULL; // in libevent, automatically removed..
		return true;
	}

	const BaseAddress* peerAddress() {
		return &address_;
	}


private:
	HttpRequest *owner_;
	struct evhttp_request *req_;
	struct evhttp_uri* evuri_;
	std::string reqBody_;
	typedef std::map<std::string, std::vector<std::string> > MapParameter_t;
	MapParameter_t parameters_;
	volatile bool parseDone_;
	std::string path_;
	IPv4Address address_;
};

}

