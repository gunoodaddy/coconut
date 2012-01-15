#include "Coconut.h"
#include <BaseProtocol.h>
#include <VirtualTransportHelper.h>

namespace coconut { namespace protocol {

bool BaseProtocol::processWrite(boost::shared_ptr<BaseVirtualTransport> transport) {

	if(parent_protocol_) {
		if(false == parent_protocol_->processWrite(transport))
			return false;
	} else if(parent_protocol_shared_ptr_) {
		if(false == parent_protocol_shared_ptr_->processWrite(transport))
			return false;
	}

	LOG_DEBUG("BaseProtocol::processWrite %d %s : %d\n", turnOnWrite_, className(), writebuffer_->remainingSize());

	if(turnOnWrite_ && writebuffer_->remainingSize() > 0) {
		boost::int32_t pos = transport->write(writebuffer_->currentPtr(), writebuffer_->remainingSize());
		if(pos != (int)writebuffer_->remainingSize()) {
			throw ProtocolException("processWrite not write all buffer to transport");
		} else {
			writebuffer_->fastforward(pos);
		}
	}
	return true;
}

boost::shared_ptr<BufferedTransport> BaseProtocol::payloadBuffer() {
	boost::shared_ptr<BufferedTransport> payload;
	if(parent_protocol_) {
		payload = parent_protocol_->payloadBuffer();
	} else if(parent_protocol_shared_ptr_) {
		payload = parent_protocol_shared_ptr_->payloadBuffer();
	} else {
		payload = buffer_;
	}
	return payload;
}

void BaseProtocol::resetReadingBufferToRemainingBuffer() {

	std::string rest;
	rest.assign((const char*)remainingBufferPtr(), remainingBufferSize());

	resetBuffer();
	addToReadingBuffer(rest.c_str(), rest.size());
}


//------------------------------------------------------------------------------------------------

bool StringListProtocol::processRead(boost::shared_ptr<BaseVirtualTransport> transport) {
	if(callParentProcessRead(transport) == false)
		return false;

	try {
		switch(state_) {
			case Init:
			case Begin:
				payload_pos_ += VirtualTransportHelper::readInt32(transport, listSize_);
				state_ = Contents;
			case Contents:
				do {
					if(listSize_ > (int)list_.size()) {
						std::string string;
						payload_pos_ += VirtualTransportHelper::readStringFast(transport, string);
						list_.push_back(string);
						if(listSize_ == (int)list_.size()) {
							state_ = End;
							break;
						}
					} else {
						state_ = End;
						break;
					}
				} while(1);
			case End:
				return true;
		}
	} catch(SocketException &e) {
		(void)e;	
	} catch(ProtocolException &e) {
		(void)e;	
	}
	return true;
}

bool StringListProtocol::processSerialize(size_t bufferSize) {
	resetWritingBuffer();
	VirtualTransportHelper::writeStringList(writebuffer_, list_);

	return callParentProcessSerialize(writebuffer_->totalSize() + bufferSize);
}

	
} }

