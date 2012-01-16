#include "Coconut.h"
#include "FrameProtocol.h"
#include "Exception.h"

namespace coconut { namespace protocol {

bool FrameProtocol::processRead(boost::shared_ptr<BaseVirtualTransport> transport) {
	try {
		switch(state_) {
			case Init:
			case Command: 
				{
					boost::int32_t cmd;
					payload_pos_ += VirtualTransportHelper::readInt32(transport, cmd);
					header_.setCommand(cmd);
					state_ = TransationID;
				}
			case TransationID: 
				{
					boost::int32_t trid;
					payload_pos_ += VirtualTransportHelper::readInt32(transport, trid);
					header_.setTransactionId(trid);
					state_ = Checksum;
				}
			case Checksum:
				{
					boost::int32_t checksum;
					payload_pos_ += VirtualTransportHelper::readInt32(transport, checksum);
					// TODO check checksum..
					if(0) 
						break;
					header_.setChecksum(checksum);
					state_ = Length;
				}
			case Length:
				{
					boost::int32_t length;
					payload_pos_ += VirtualTransportHelper::readInt32(transport, length);
					header_.setLength(length);
					state_ = Payload;
				}
			case Payload: 
				{
//#define PROTOCOL_READ_FROM_SOCKET
#ifdef PROTOCOL_READ_FROM_SOCKET
					int remain = header_.length() - payload_pos_ - buffer_->totalSize();
					if(remain > 0) {
						// if remain > 0, it means that you need to read from socket..
						// and [buffer_] store only "payload" data..
						// therefore [transport] variable must be "BaseSocket" class instance for read from socket..
						// if remain < 0, already you read all bytes for parsing packet..
						// thease options are decided in ClientController::onSocket_ReadEvent
						assert(strcmp(transport->className(),  "BaseSocket") == 0);
						char tempBuf[IOBUF_LEN];
						int nread = transport->read((void *)tempBuf, remain);
						//printf("%d %d %d %d %d\n", remain, nread,  header_.length() , payload_pos_ , buffer_->totalSize());
						if(nread > 0)
							buffer_->write(tempBuf, nread);
						if(buffer_->totalSize() < header_.length() - payload_pos_)
							break; // need more data..
					}
#else
					int remain = header_.length() - buffer_->totalSize();
					if(remain > 0)
						break; // need mode data..
#endif
					initHeader_ = header_;
					payload_pos_on_buffer_ = buffer_->readPos();	// must use readPos() return value..
					state_ = Complete;
					//printf("## parsing complete : remain %d length %d total %d bufremain %d initheaderlen %d pos %d readPaySize %d\n", remain, header_.length(), buffer_->totalSize(), buffer_->remainingSize(), initHeader_.length(), payload_pos_, readPayloadSize());
				}
			case Complete:
				return true;
		} 
	} catch(ProtocolException &e) {
		(void)e;
		// nothing to do..
	} catch(SocketException &e) {
		(void)e;
		// nothing to do..
	}
	return false;
}


bool FrameProtocol::processWrite(boost::shared_ptr<BaseVirtualTransport> transport) {

	BaseProtocol::processWrite(transport);	// FrameProtocol do not have parent_protocol_! so just write only my buffer..
	
	if(payload_protocol_) {
		payload_protocol_->processWrite(transport);
	} else if(payload_protocol_shared_ptr_) {
		payload_protocol_shared_ptr_->processWrite(transport);
	}

	return true;
}


bool FrameProtocol::processSerialize(size_t bufferSize) {
	resetWritingBuffer();

	// Frame Header..
	header_.makeChecksum(payloadPtr(), payloadSize());

	if(payload_.size() > 0) {
		header_.serialize(writebuffer_, bufferSize + payload_.size());
		writebuffer_->write(payload_.c_str(), payload_.size());
	} else if(payload_protocol_) {
		payload_protocol_->processSerialize();
		header_.serialize(writebuffer_, bufferSize + payload_protocol_->writingBufferSize());
	} else if(payload_protocol_shared_ptr_) {
		payload_protocol_shared_ptr_->processSerialize();
		header_.serialize(writebuffer_, bufferSize + payload_protocol_shared_ptr_->writingBufferSize());
	} else {
		header_.serialize(writebuffer_, bufferSize);
	}

	return true;
}


const void * FrameProtocol::remainingBufferPtr() {
	if(isReadComplete())
		return (const char*)buffer_->currentPtr() + (initHeader_.length() - payload_pos_ - readPayloadSize());
	return BaseProtocol::remainingBufferPtr();
}


size_t FrameProtocol::remainingBufferSize() {
	if(isReadComplete()) {
		//printf("===========> FrameProtocol::remainingBufferSize : %d %d %d %d %d\n", buffer_->remainingSize(), initHeader_.length(), payload_pos_, readPayloadSize(), buffer_->remainingSize() - (initHeader_.length() - payload_pos_ - readPayloadSize()));
		return buffer_->remainingSize() - (initHeader_.length() - payload_pos_ - readPayloadSize());
	}
	return BaseProtocol::remainingBufferSize();
}


const void * FrameProtocol::payloadPtr() {
	// for performance, do not copy to payload_ string.
	if(payload_.size() > 0)
		return payload_.c_str();
	else if(isReadComplete()) {
		if(payload_pos_on_buffer_ < payloadBuffer()->totalSize())
			return (const char*)payloadBuffer()->basePtr() + payload_pos_on_buffer_;
	}
	return NULL;
}


size_t FrameProtocol::payloadSize() {
	if(payload_.size() > 0)
		return payload_.size();
	else if(isReadComplete()) {
		int expacted = initHeader_.length() - payload_pos_;
		if(expacted <= (int)payloadBuffer()->totalSize())
			return expacted;
	}
	return 0;
}

}}

