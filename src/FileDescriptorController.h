#pragma once

#include "ClientController.h"
#include "BaseProtocol.h"
#include "FileDescriptorProtocol.h"

namespace coconut {

class COCONUT_API FileDescriptorController : public ClientController { 
private:
	class COCONUT_API FDProtocolFactory : public protocol::BaseProtocolFactory {
	public:
		boost::shared_ptr<protocol::BaseProtocol> makeProtocol() {
			boost::shared_ptr<protocol::FileDescriptorProtocol> protocol(new protocol::FileDescriptorProtocol);
			return protocol;
		}
	};
	void _onPreInitialized() {
		boost::shared_ptr<FDProtocolFactory> factory(new FDProtocolFactory);
		setProtocolFactory(factory);
		
		BaseController::_onPreInitialized();
	}

	void onSocket_ReadEvent(int fd);

	void onReceivedProtocol(boost::shared_ptr<protocol::BaseProtocol> protocol);

public:
	void writeDescriptor(int fd);

protected:
	// DescriptorController callback event
	virtual void onDescriptorReceived(int fd) = 0;
};

} // end of namespace coconut
