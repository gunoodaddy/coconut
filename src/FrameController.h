#pragma once

#include "ClientController.h"
#include "BaseProtocol.h"
#include "FrameProtocol.h"

namespace coconut {

class COCONUT_API FrameController : public ClientController {
private:
	class FrameProtocolFactory : public protocol::BaseProtocolFactory {
	public:
		boost::shared_ptr<protocol::BaseProtocol> makeProtocol() {
			boost::shared_ptr<protocol::FrameProtocol> prot(new protocol::FrameProtocol);
			return prot;
		}
	};
	void _onPreInitialized() {
		boost::shared_ptr<FrameProtocolFactory> factory(new FrameProtocolFactory);
		setProtocolFactory(factory);
		
		BaseController::_onPreInitialized();
	}

	void onReceivedProtocol(boost::shared_ptr<protocol::BaseProtocol> protocol);

public:
	void writeFrame(const protocol::FrameHeader &header, boost::shared_ptr<protocol::BaseProtocol> protocol);
	void writeFrame(const protocol::FrameHeader &header, protocol::BaseProtocol *protocol);
	void writeFrame(const protocol::FrameHeader &header, boost::shared_ptr<BufferedTransport> buffer);
	void writeFrame(const protocol::FrameHeader &header, const void *payload, size_t size);

protected:
	// FrameController callback event
	virtual void onFrameReceived(boost::shared_ptr<protocol::FrameProtocol> prot) = 0;
};

} // end of namespace coconut
