#pragma once

#include "ClientController.h"
#include "BaseProtocol.h"
#include "LineProtocol.h"

namespace coconut {

class COCONUT_API LineController : public ClientController {
private:
	class LineProtocolFactory : public protocol::BaseProtocolFactory {
	public:
		boost::shared_ptr<protocol::BaseProtocol> makeProtocol() {
			boost::shared_ptr<protocol::LineProtocol> line(new protocol::LineProtocol);
			return line;
		}
	};
	void _onPreInitialized() {
		boost::shared_ptr<LineProtocolFactory> factory(new LineProtocolFactory);
		setProtocolFactory(factory);
		
		BaseController::_onPreInitialized();
	}

	void onReceivedProtocol(boost::shared_ptr<protocol::BaseProtocol> protocol);

public:
	void writeLine(const std::string &line);

protected:
	// LineController callback event
	virtual void onLineReceived(const char *line) = 0;

private:
	std::string buffer_;
};

} // end of namespace coconut
