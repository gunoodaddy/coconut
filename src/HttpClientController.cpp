#include "Coconut.h"
#include "IOService.h"
#include "HttpClientController.h"
#include "HttpClient.h"

namespace coconut {

HttpClientController::~HttpClientController() {

}

boost::shared_ptr<IOService> HttpClientController::ioService() {
	return client_->ioService();
}

BaseIOServiceContainer *HttpClientController::ioServiceContainer() {
	return client_->ioService()->ioServiceContainer();
}


void HttpClientController::onHttpClient_Response(int rescode) { 
	onResponse(rescode);

	eventGotResponse()->fireObservers(shared_from_this(), 0);
}

void HttpClientController::onHttpClient_Error(HttpClient::ErrorCode errorcode) {
	onError(errorcode);

	eventOccuredError()->fireObservers(shared_from_this(), errorcode);
}

void HttpClientController::onHttpClient_ReceivedChunked(int receivedsize) { 
	onReceivedChucked(receivedsize);
}

} // end of namespace coconut
