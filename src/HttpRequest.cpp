#include "Coconut.h"
#include "IOService.h"
#include "HttpRequest.h"
#include "Exception.h"
#include "Logger.h"
#include <event2/dns.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/keyvalq_struct.h>
#include <event2/buffer.h>
#include <fcntl.h>
#include <map>
#include <vector>
#if defined(HAVE_SYS_QUEUE_H)
#include <sys/queue.h>
#endif
#if defined(WIN32)
#include <io.h>
#endif

#ifndef TAILQ_FIRST
#define TAILQ_FIRST(head)       ((head)->tqh_first)
#endif
#ifndef TAILQ_END
#define TAILQ_END(head)         NULL
#endif
#ifndef TAILQ_NEXT
#define TAILQ_NEXT(elm, field)      ((elm)->field.tqe_next)
#endif

#ifndef TAILQ_FOREACH
#define TAILQ_FOREACH(var, head, field)                 \
	for ((var) = TAILQ_FIRST(head);                 \
			(var) != TAILQ_END(head);                  \
			(var) = TAILQ_NEXT(var, field))
#endif

namespace coconut {

class HttpRequestImpl {
public:
	HttpRequestImpl(HttpRequest *owner, 
					boost::shared_ptr<IOService> ioService, 
					HttpMethodType method, 
					const char *uri, 
					const HttpParameter *param, 
					int timeout) 
		: owner_(owner)
		, ioService_(ioService)
		, state_(HttpRequest::Prepare)
		, evcon_(NULL)
		, dnsbase_(NULL)
		, req_(NULL)
		, evuri_(NULL)
		, responsebuffer_(NULL)
		, method_(method)
		, uri_(uri)
		, paramTemp_(param)
		, timeout_(timeout)
		, chunkmode_(true)
		, multipart_(true)
		, responseCode_(HTTP_INTERNAL) {

		state_ = HttpRequest::Prepare;
	}

	~HttpRequestImpl() {
		LOG_TRACE("~HttpRequest : %p", this);
		cleanUp(true);
	}

public:
	static unsigned long long get_file_size(const char *path) {
#if defined(WIN32)
		assert(false && "TODO please make get_file_size function for WIN32");
		return 0;
#else
		struct stat filestatus;
		int ret = stat(path, &filestatus );
		if(ret != 0)
			return 0;
		return filestatus.st_size;
#endif
	}

	static std::string get_name_of_path(const char *path) {
#if defined(WIN32)
		assert(false && "TODO please make get_name_of_path function for WIN32");
		return "";
#else
		std::string path_(path);
		size_t pos = path_.rfind("/");
		if(pos == std::string::npos)
			return path_;
		if(pos == path_.size() - 1)
			return "";

		return path_.substr(pos+1, path_.size() - pos - 1);
#endif
	}

public:
	boost::shared_ptr<IOService> ioService() {
		return ioService_;
	}

	void cleanUp(bool deleteReqFlag) {
		ScopedIOServiceLock(ioService_);

		state_ = HttpRequest::Prepare;

		if(evcon_) {
			evhttp_connection_free(evcon_);
			evcon_ = NULL;
			req_ = NULL;	// must be set null. freed in evhttp_connection_free
		}
		if(req_) {
			if(deleteReqFlag)
				evhttp_request_free(req_);
			req_ = NULL;
		}
		if(evuri_) {
			evhttp_uri_free(evuri_);
			evuri_ = NULL;
		}
		if(dnsbase_) {
			evdns_base_free(dnsbase_, 0);
			dnsbase_ = NULL;
		}
		if(responsebuffer_) {
			evbuffer_free(responsebuffer_);
			responsebuffer_ = NULL;
		}
	}

	void cancelRequest() {
		ScopedIOServiceLock(ioService_);
		if(req_) {
			LOG_DEBUG("Http Request Cancel!!\n");
			assert(req_ && "now state is Requesting but req_ is NULL");

			evhttp_cancel_request(req_);	

			fire_onHttpRequest_Error(HttpRequest::Canceled);

			state_ = HttpRequest::Prepare;
		}
	}

	const void* responseBody() {
		return (const void*)responseBody_.c_str();
	}

	int responseBodySize() {
		return responseBody_.size();
	}

	int responseCode() {
		return responseCode_;
	}

	std::string findHeader(const char *key) {
		ScopedIOServiceLock(ioService_);
		std::map<std::string, std::string>::iterator it = inputHeaders_.find(key);
		if(it != inputHeaders_.end())
			return it->second;
		return "";
	}

	void request(HttpMethodType method, const char *uri, const HttpParameter * param, int timeout) {
		ScopedIOServiceLock(ioService_);
		if(HttpRequest::Prepare != state_) {
			throw IllegalStateException("Already http requested, you need to call cancelRequest");
		}

		// setting..
		uri_ = uri;
		method_ = method;
		paramTemp_ = param;
		timeout_ = timeout;

		requestInternal();
	}

	void request() {
		ScopedIOServiceLock(ioService_);
		if(HttpRequest::Prepare != state_) {
			throw IllegalStateException("Already http requested, you need to call cancelRequest");
		}
		requestInternal();
	}

	void parseParameter() {
		struct evkeyvalq args;
		struct evkeyval *get;
		LOG_DEBUG("REQUEST URL = %s\n", uri_.c_str());
		evhttp_parse_query(uri_.c_str(), &args);

		// from user parameter 
		if(paramTemp_) {
			param_ = *paramTemp_;
		}
		// from uri..
		TAILQ_FOREACH(get, &args, next) {
			param_.addParameter(get->key, get->value);
		}
	}

	void makeUriContext() {
		evuri_ = evhttp_uri_parse_with_flags (uri_.c_str(), 0);
		assert(evuri_ && "evhttp_uri can not be allocated");
		LOG_DEBUG("REQUEST URI INFORMATION => %s:%d, %s, %s\n", 
			evhttp_uri_get_host(evuri_), 
			evhttp_uri_get_port(evuri_), 
			evhttp_uri_get_path(evuri_),
			evhttp_uri_get_query(evuri_));
	}

	void makeHttpRequest() {
		req_ = evhttp_request_new (http_done_cb, this);
		assert(req_ && "evhttp_request can not be allocated");

		if(chunkmode_) {
			evhttp_request_set_chunked_cb(req_, http_chuncked_cb);
			responsebuffer_ = evbuffer_new();
		}
	}

	void makeHeaderAndBody() {
		_makeBasicHeader();

		int contentlength = 0;

		if(HTTP_POST == method_) {
			_makeMultipartBoundary();
			_makePostHeader();
			contentlength = _makePostBody();
		} 

		_makeContentLengthHeader(contentlength);
	}

	void _makeBasicHeader() {
		// Basic Header
		evhttp_add_header(req_->output_headers, "Host", evhttp_uri_get_host(evuri_));

		// Header from user 
		for(int i = 0; i < param_.count(); i++) {
			const HttpParameter::parameter_t& p = param_.indexOf(i);
			if(HttpParameter::HEADER_TYPE != p.type)
				continue;
			evhttp_add_header(req_->output_headers, p.key.c_str(), p.value.c_str());
		}
	}

	void _makeContentLengthHeader(int size) {
		char contentlengthStr[20] = {0, };
		sprintf(contentlengthStr, "%d", size);
		evhttp_add_header(req_->output_headers, "Content-Length", contentlengthStr); 
	}

	void _makePostHeader() {
		if(multipart_) {
			char header[1024] = {0, };
			sprintf(header, "multipart/form-data; boundary=%s", boundary_.c_str());
			evhttp_add_header(req_->output_headers, "Content-Type", header);
		} else {
			evhttp_add_header(req_->output_headers, "Content-Type", "application/x-www-form-urlencoded");
		}
	}

	void _makeMultipartBoundary() {
		char temp[1024] = {0, };
		srand((unsigned int)time(NULL));
		sprintf(temp, "--%x%x%x%x%x%x%x%x%x%x%x%x%x",
				rand() % 16, rand() % 16, rand() % 16, rand() % 16, rand() % 16,
				rand() % 16, rand() % 16, rand() % 16, rand() % 16, rand() % 16,
				rand() % 16, rand() % 16, rand() % 16);

		boundary_ = temp;
	}

	int _makePostBody() {
		if(multipart_) {
			for(int i = 0; i < param_.count(); i++) {
				const HttpParameter::parameter_t& p = param_.indexOf(i);
				switch(p.type) {
					case HttpParameter::HEADER_TYPE:
						continue;	// skipped..
					case HttpParameter::STRING_TYPE:
						evbuffer_add_printf(req_->output_buffer, "--%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n",
								boundary_.c_str(), p.key.c_str());
						evbuffer_add(req_->output_buffer, p.value.c_str(), p.value.size());
						evbuffer_add(req_->output_buffer, "\r\n", 2);
						continue;	
					case HttpParameter::FILE_TYPE:
#if defined(WIN32)
						int fd = _open(p.value.c_str(), O_RDONLY);
#else
						int fd = open(p.value.c_str(), O_RDONLY);
#endif
						if(fd <= 0)
							continue;

						std::string filename(get_name_of_path(p.value.c_str()));
						unsigned long long size = get_file_size(p.value.c_str());
						// if file size exceed 2098790 byte, failed to send..
						// --->> IT'S NOT BUG! php.ini (upload_max_filesize = 2M)... orz..
						evbuffer_add_printf(req_->output_buffer, 
							"--%s\r\nContent-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\nContent-Type: \"%s\"\r\n\r\n",
							boundary_.c_str(), p.key.c_str(), filename.c_str(), "application/octet-stream");
						evbuffer_add_file(req_->output_buffer, fd, 0, size);
						evbuffer_add(req_->output_buffer, "\r\n", 2);

#if defined(WIN32)
						_close(fd);
#else
						close(fd);
#endif
						continue;	
				}
			}
		} else {
			std::string s(_makeGetMethodBody());
			evbuffer_add(req_->output_buffer, s.c_str(), s.size());
		}

		LOG_DEBUG("BODY SIZE %d\n", evbuffer_get_length(req_->output_buffer));
		return evbuffer_get_length(req_->output_buffer);
	}

	void _printOutputBuffer() {
		size_t buffer_len = evbuffer_get_length(req_->output_buffer);
		const void *data = evbuffer_pullup(req_->output_buffer, buffer_len);
		char *buffer = new char[buffer_len+1];
		memset(buffer, 0, buffer_len+1);
		memcpy(buffer, data, buffer_len);
		LOG_DEBUG("BODY\n%s\n", buffer);
		delete [] buffer;
	}

	std::string makeRequestUri() {
		std::string result;
		result +=  evhttp_uri_get_path(evuri_);	
		if(HTTP_GET == method_) {
			result += "?";
			result += _makeGetMethodBody();
		}

		LOG_DEBUG("QUERY %s\n", result.c_str());
		return result;
	}

	std::string _makeGetMethodBody() {
		std::string result;
		bool first = true;
		for(int i = 0; i < param_.count(); i++) {
			const HttpParameter::parameter_t& p = param_.indexOf(i);
			if(!first)
				result += "&";
			if(HttpParameter::STRING_TYPE != p.type)
				continue;
			result += p.key;
			result += "=";
			char *enc = evhttp_uriencode(p.value.c_str(), p.value.size(), 0);
			result += enc;
			free(enc);
			first = false;
		}
		return result;
	}

	void makeHttpConnection() {
		int port = evhttp_uri_get_port(evuri_);
		if(port < 0)
			port = 80;
		LOG_TRACE("Http Connection Making.. : %s:%d", evhttp_uri_get_host(evuri_), port);
		evcon_ = evhttp_connection_base_new(ioService_->coreHandle(), dnsbase_, evhttp_uri_get_host(evuri_), port);
		assert(evcon_ && "evhttp_connection can not be allocated");
		
		evhttp_connection_set_timeout(evcon_, timeout_); 
	}

	void startRequest(std::string &uri) {
		dnsbase_ = evdns_base_new(ioService_->coreHandle(), 1);
		assert(dnsbase_ && "evdns_base can not be allocated");

		evhttp_make_request(evcon_, req_, method_ == HTTP_POST ? EVHTTP_REQ_POST : EVHTTP_REQ_GET, uri.c_str());

		state_ = HttpRequest::Requesting;
	}

	void makeResultHeaders() {
		struct evkeyvalq *queue = req_->input_headers;
		struct evkeyval *get;

		TAILQ_FOREACH(get, queue, next) {
			inputHeaders_.insert(std::map<std::string, std::string>::value_type(get->key, get->value));
		}
	}

	void makeResultBody() {
		struct evbuffer *buffer;
		if(chunkmode_) {
			buffer = responsebuffer_;
		} else {
			buffer = req_->input_buffer;
		}
		size_t buffer_len = evbuffer_get_length(buffer);
		const void *data = evbuffer_pullup(buffer, buffer_len);
		responseBody_.assign((const char *)data, buffer_len);
	}

	void fire_onHttpRequest_ReceivedChunked() {
		state_ = HttpRequest::ReceivingResponse;

		evbuffer_add_buffer(responsebuffer_, evhttp_request_get_input_buffer(req_));

		owner_->eventHandler()->onHttpRequest_ReceivedChunked(evbuffer_get_length(responsebuffer_));
	}

	// TODO reading progress feature supported,
	// but "writing" progress feature NOT supported
	void fire_onHttpRequest_Error(HttpRequest::ErrorCode errorcode) {
		LOG_DEBUG("HttpRequest Got error");

		cleanUp(false); // automatically freed in libevent after callback

		owner_->eventHandler()->onHttpRequest_Error(errorcode);
	}

	void fire_onHttpRequest_Error() {
		HttpRequest::ErrorCode errorcode;
		if(state_ == HttpRequest::Requesting) {
			if(chunkmode_ == false) {
				errorcode = HttpRequest::ResponseError;
				// TODO How to divide error and connection timeout...
			} else {
				errorcode = HttpRequest::ConnectionTimeout;
			}
		} else {
			errorcode = HttpRequest::ResponseError; // TODO how to get more detail error..
		}
		fire_onHttpRequest_Error(errorcode);
	}

	void fire_onHttpRequest_Response() {
		responseCode_ = req_->response_code;
		if (responseCode_ == HTTP_OK) {
			makeResultHeaders();
			makeResultBody();
		}

		cleanUp(false); // automatically freed in libevent after callback
		owner_->eventHandler()->onHttpRequest_Response(responseCode_);
	}

private:
	static void http_chuncked_cb(struct evhttp_request *req, void *arg) {
		HttpRequestImpl *SELF = (HttpRequestImpl *)arg;

		assert(req && "chunked callback called but req is NULL");
		SELF->fire_onHttpRequest_ReceivedChunked();
	}

	static void http_done_cb(struct evhttp_request *req, void *arg) {
		HttpRequestImpl *SELF = (HttpRequestImpl *)arg;

		if(req)
			SELF->fire_onHttpRequest_Response();
		else
			SELF->fire_onHttpRequest_Error();
	}

private:
	void requestInternal() {
		parseParameter();
		makeUriContext();
		makeHttpRequest();
		makeHeaderAndBody();
		std::string requesturi(makeRequestUri());
		makeHttpConnection();

		startRequest(requesturi);
	}

private:
	HttpRequest *owner_;
	boost::shared_ptr<IOService> ioService_;
	HttpRequest::RequestState state_;
	struct evhttp_connection *evcon_;
	struct evdns_base *dnsbase_;
	struct evhttp_request *req_;
	struct evhttp_uri* evuri_;
	struct evbuffer *responsebuffer_;
	HttpMethodType method_;
	std::string uri_;
	const HttpParameter *paramTemp_;
	int timeout_;
	std::string responseBody_;
	HttpParameter param_;
	bool chunkmode_;
	bool multipart_;
	std::string boundary_;
	std::map<std::string, std::string> inputHeaders_;
	int responseCode_;
};

//--------------------------------------------------------------------------------------------------------------

HttpRequest::HttpRequest(boost::shared_ptr<IOService> ioService, 
						 HttpMethodType method, 
						 const char *uri, 
						 const HttpParameter *param, 
						 int timeout) {
	impl_ = new HttpRequestImpl(this, ioService, method, uri, param, timeout);
}

HttpRequest::~HttpRequest() {
	delete impl_;
}

boost::shared_ptr<IOService> HttpRequest::ioService() {
	return impl_->ioService();
}

void HttpRequest::cleanUp(bool deleteReqFlag) {
	impl_->cleanUp(deleteReqFlag);
}

void HttpRequest::cancelRequest() {
	impl_->cancelRequest();
}

const void* HttpRequest::responseBody() {
	return impl_->responseBody();
}

int HttpRequest::responseBodySize() {
	return impl_->responseBodySize();
}

int HttpRequest::responseCode() {
	return impl_->responseCode();
}

std::string HttpRequest::findHeader(const char *key) {
	return impl_->findHeader(key);
}

void HttpRequest::request(HttpMethodType method, const char *uri, const HttpParameter * param, int timeout) {
	impl_->request(method, uri, param, timeout);
}

void HttpRequest::request() {
	impl_->request();
}

}
