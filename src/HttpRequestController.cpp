#include "Coconut.h"
#include "IOService.h"
#include "HttpRequestController.h"
#include "HttpRequest.h"

namespace coconut {

HttpRequestController::~HttpRequestController() {

}

boost::shared_ptr<IOService> HttpRequestController::ioService() {
	return request_->ioService();
}

BaseIOServiceContainer *HttpRequestController::ioServiceContainer() {
	return request_->ioService()->ioServiceContainer();
}


void HttpRequestController::onHttpRequest_Response(int rescode) { 
	onResponse(rescode);

	eventGotResponse()->fireObservers(shared_from_this(), 0);
}

void HttpRequestController::onHttpRequest_Error(HttpRequest::ErrorCode errorcode) {
	onError(errorcode);

	eventOccuredError()->fireObservers(shared_from_this(), errorcode);
}

void HttpRequestController::onHttpRequest_ReceivedChunked(int receivedsize) { 
	onReceivedChucked(receivedsize);
}

} // end of namespace coconut
