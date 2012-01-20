#pragma once

#include "ClientController.h"
#include "BaseProtocol.h"
#include "JSONProtocol.h"

namespace coconut {

class COCONUT_API JSONController : public ClientController {
private:
	class JSONProtocolFactory : public protocol::BaseProtocolFactory {
	public:
		boost::shared_ptr<protocol::BaseProtocol> makeProtocol() {
			boost::shared_ptr<protocol::JSONProtocol> json(new protocol::JSONProtocol);
			return json;
		}
	};
	void _onPreInitialized() {
		boost::shared_ptr<JSONProtocolFactory> factory(new JSONProtocolFactory);
		setProtocolFactory(factory);
		BaseController::_onPreInitialized();
	}

	void onReceivedProtocol(boost::shared_ptr<protocol::BaseProtocol> protocol);

public:
	void writeJSON(const std::string &json);

protected:
	// JSONController callback event
	virtual void onJSONReceived(const char *json) = 0;

private:
	std::string buffer_;
};

} // end of namespace coconut
