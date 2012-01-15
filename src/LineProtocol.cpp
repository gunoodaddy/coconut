#include "Coconut.h"
#include "LineProtocol.h"
#include "VirtualTransportHelper.h"

namespace coconut { namespace protocol {

bool LineProtocol::processSerialize(size_t bufferSize) {
	resetWritingBuffer();
	VirtualTransportHelper::writeBinary(writebuffer_, line_.c_str(), line_.size());
	VirtualTransportHelper::writeBinary(writebuffer_, "\r\n", 2);

	if(callParentProcessSerialize(bufferSize + writebuffer_->totalSize()) == false)
		return false;

	return true;
}

bool LineProtocol::processRead(boost::shared_ptr<BaseVirtualTransport> transport) {
	if(callParentProcessRead(transport) == false)
		return false;

	try {
		do {
			boost::int8_t byte;
			if(VirtualTransportHelper::readInt8(transport, byte) == 1) {
				if(byte != '\n') {
					if(byte != '\r')
						line_.append(1, (char)byte);
				} else {
					LOG_DEBUG("LineProtocol line received : [%s]", line_.c_str()); 
					readComplete_ = true;
					return true;
				}
			} else {
				break;
			}
		} while(1); 
	} catch (SocketException &e) {
		(void)e;
	} catch (ProtocolException &e) {
		(void)e;
	}
	return false;
}

}}

