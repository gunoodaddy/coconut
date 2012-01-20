#include "Coconut.h"
#include "JSONController.h"

using namespace coconut::protocol;

namespace coconut {

	
void JSONController::writeJSON(const std::string &json) {
	boost::shared_ptr<JSONProtocol> jsonProt 
		= boost::static_pointer_cast<JSONProtocol>(protocolFactory_->makeProtocol());

	jsonProt->setJSON(json);
	jsonProt->processSerialize();
	jsonProt->processWrite(socket());
	
}

void JSONController::onReceivedProtocol(boost::shared_ptr<protocol::BaseProtocol> protocol) {
	boost::shared_ptr<JSONProtocol> json = boost::static_pointer_cast<JSONProtocol>(protocol);
	LOG_TRACE("JSONController json received : %s", json->jsonPtr());
	onJSONReceived(json->jsonPtr());
}

} // end of namespace coconut
