#pragma once

#include "BaseController.h"
#include "HttpClient.h"

namespace coconut {

class IOService;

class COCONUT_API HttpClientController : public BaseController
		, public HttpClient::EventHandler {
public:
	HttpClientController() : ioServiceContainer_(NULL) { 
	}

	virtual ~HttpClientController();

	// strategy pattern
	void setHttpClient(boost::shared_ptr<HttpClient> client) {
		client_ = client;

		_onPreInitialized();
	}

	boost::shared_ptr<HttpClient> httpClient() {
		return client_;
	}

	boost::shared_ptr<IOService> ioService();
	BaseIOServiceContainer *ioServiceContainer();

private:
	void onHttpClient_Response(int rescode);
	void onHttpClient_Error(HttpClient::ErrorCode errorcode);
	void onHttpClient_ReceivedChunked(int receivedsize);

protected:
	// HttpClientController callback event
	virtual void onResponse(int rescode) { }
	virtual void onError(HttpClient::ErrorCode errorcode) { }
	virtual void onReceivedChucked(int receivedsize) { }

private:
	BaseIOServiceContainer *ioServiceContainer_;
	boost::shared_ptr<HttpClient> client_;
};

} // end of namespace coconut
