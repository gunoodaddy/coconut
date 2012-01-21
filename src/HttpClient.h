#pragma once

#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#endif

namespace coconut {

class IOService;
class HttpClientImpl;

class COCONUT_API HttpParameter {
public:
	enum parameterType {
		HEADER_TYPE,
		FILE_TYPE,
		STRING_TYPE,
	};

	typedef struct parameter_t {
		parameterType type;
		std::string key;
		std::string value;
	}parameter_t;

	void addHeader(const char *key, const char *value) {
		parameter_t param;
		param.type = HEADER_TYPE;
		param.key = key;
		param.value = value;
		parameters_.push_back(param);
	}
	void addParameter(const char *key, const char *value, int valuesize) {
		parameter_t param;
		param.type = STRING_TYPE;
		param.key = key;
		param.value.assign(value, valuesize);
		parameters_.push_back(param);
	}
	void addParameter(const char *key, const char *value) {
		addParameter(key, value, strlen(value));
	}
	void addParameter(const char *key, int value) {
		char buffer[256] = {0, };
		sprintf(buffer, "%d", value);
		addParameter(key, buffer);
	}
	void addFile(const char *key, const char *filePath) {
		parameter_t param;
		param.type = FILE_TYPE;
		param.key = key;
		param.value = filePath;
		parameters_.push_back(param);
	}
	const parameter_t & indexOf(int index) const {
		return parameters_[index];
	}
	int count() const {
		return parameters_.size();
	}
private:
	std::vector<parameter_t> parameters_;
};


class COCONUT_API HttpClient {
public:
	HttpClient(boost::shared_ptr<IOService> ioService, 
				HttpMethodType method, 
				const char *uri, 
				const HttpParameter *param, 
				int timeout);
	~HttpClient();

	enum ErrorCode {
		None,
		ConnectionTimeout,
		ResponseError,
		Canceled,
	};

	enum RequestState {
		Prepare,
		Requesting,
		ReceivingResponse,
	};

	class EventHandler
	{
	public:
		virtual ~EventHandler() { }
		virtual void onHttpClient_Response(int rescode) { }
		virtual void onHttpClient_Error(ErrorCode errorcode) { }
		virtual void onHttpClient_ReceivedChunked(int receivedsize) { }
	};

public:
	boost::shared_ptr<IOService> ioService();

	void setEventHandler(EventHandler *handler) {
		handler_ = handler;
	}

	EventHandler * eventHandler() {
		return handler_;
	}

	void cleanUp(bool deleteReqFlag);
	void cancelRequest();

	const void* responseBody();
	int responseBodySize();
	int responseCode();

	std::string findHeader(const char *key);

	void request(HttpMethodType method, const char *uri, const HttpParameter * param, int timeout);
	void request();

private:
	HttpClientImpl *impl_;
	EventHandler *handler_;
};

}
