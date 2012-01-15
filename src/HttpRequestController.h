#pragma once

#include "BaseController.h"
#include "HttpRequest.h"

namespace coconut {

class IOService;

class COCONUT_API HttpRequestController : public BaseController
		, public HttpRequest::EventHandler {
public:
	HttpRequestController() : ioServiceContainer_(NULL) { 
	}

	virtual ~HttpRequestController();

	// strategy pattern
	void setHttpRequest(boost::shared_ptr<HttpRequest> request) {
		request_ = request;

		_onPreInitialized();
	}

	boost::shared_ptr<HttpRequest> httpRequest() {
		return request_;
	}

	boost::shared_ptr<IOService> ioService();
	BaseIOServiceContainer *ioServiceContainer();

private:
	void onHttpRequest_Response(int rescode);
	void onHttpRequest_Error(HttpRequest::ErrorCode errorcode);
	void onHttpRequest_ReceivedChunked(int receivedsize);

protected:
	// HttpRequestController callback event
	virtual void onResponse(int rescode) { }
	virtual void onError(HttpRequest::ErrorCode errorcode) { }
	virtual void onReceivedChucked(int receivedsize) { }

private:
	BaseIOServiceContainer *ioServiceContainer_;
	boost::shared_ptr<HttpRequest> request_;
};

} // end of namespace coconut
