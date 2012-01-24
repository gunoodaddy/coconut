#include "Coconut.h"
#include "LineController.h"
#include "InternalLogger.h"

using namespace coconut::protocol;

namespace coconut {

	
void LineController::writeLine(const std::string &line) {
	boost::shared_ptr<LineProtocol> lineProt 
		= boost::static_pointer_cast<LineProtocol>(protocolFactory_->makeProtocol());

	lineProt->setLine(line);
	lineProt->processSerialize();
	lineProt->processWrite(socket());
	
}

void LineController::onReceivedProtocol(boost::shared_ptr<protocol::BaseProtocol> protocol) {
	boost::shared_ptr<LineProtocol> line = boost::static_pointer_cast<LineProtocol>(protocol);
	_LOG_TRACE("LineController line received : %s", line->linePtr());
	onLineReceived(line->linePtr());
}

} // end of namespace coconut
