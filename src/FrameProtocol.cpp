/*
* Copyright (c) 2012, Eunhyuk Kim(gunoodaddy) 
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*   * Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*   * Neither the name of Redis nor the names of its contributors may be used
*     to endorse or promote products derived from this software without
*     specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#include "CoconutLib.h"
#include "FrameProtocol.h"
#include "Exception.h"

namespace coconut { namespace protocol {

bool FrameProtocol::processRead(boost::shared_ptr<BaseVirtualTransport> transport) {
	try {
		switch(state_) {
			case Init:
			case Command: 
				{
					boost::uint32_t cmd;
					payload_pos_ += VirtualTransportHelper::readInt32(transport, cmd);
					header_.setCommand(cmd);
					state_ = TransationID;
				}
			case TransationID: 
				{
					boost::uint32_t trid;
					payload_pos_ += VirtualTransportHelper::readInt32(transport, trid);
					header_.setTransactionId(trid);
					state_ = Checksum;
				}
			case Checksum:
				{
					boost::uint32_t checksum;
					payload_pos_ += VirtualTransportHelper::readInt32(transport, checksum);
					// TODO check checksum..
					if(0 /* need check logic codes */) 
						break;
					header_.setChecksum(checksum);
					state_ = Length;
				}
			case Length:
				{
					boost::uint32_t length;
					payload_pos_ += VirtualTransportHelper::readInt32(transport, length);
					header_.setLength(length);
					state_ = Payload;
				}
			case Payload: 
				{
//#define PROTOCOL_READ_FROM_SOCKET
#ifdef PROTOCOL_READ_FROM_SOCKET
					int remain = header_.length() - payload_pos_ - readBuffer_->totalSize();
					if(remain > 0) {
						// if remain > 0, it means that you need to read from socket..
						// and [readBuffer_] store only "payload" data..
						// therefore [transport] variable must be "BaseSocket" class instance for read from socket..
						// if remain < 0, already you read all bytes for parsing packet..
						// thease options are decided in ClientController::onSocket_ReadEvent
						assert(strcmp(transport->className(),  "BaseSocket") == 0);
						char tempBuf[IOBUF_LEN];
						int nread = transport->read((void *)tempBuf, remain);
						//printf("%d %d %d %d %d\n", remain, nread,  header_.length() , payload_pos_ , readBuffer_->totalSize());
						if(nread > 0)
							readBuffer_->write(tempBuf, nread);
						if(readBuffer_->totalSize() < header_.length() - payload_pos_)
							break; // need more data..
					}
#else
					int remain = header_.length() - readBuffer_->totalSize();
					if(remain > 0)
						break; // need mode data..
#endif
					initHeader_ = header_;
					payload_pos_ = readBuffer_->readPos();	// must use readPos() return value..
					state_ = Complete;
					//printf("## parsing complete : remain %d length %d total %d bufremain %d initheaderlen %d pos %d readPaySize %d\n", remain, header_.length(), readBuffer_->totalSize(), readBuffer_->remainingSize(), initHeader_.length(), payload_pos_, readPayloadSize());
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
		return (const char*)readBuffer_->currentPtr() + (initHeader_.length() - payload_pos_ - readPayloadSize());
	return BaseProtocol::remainingBufferPtr();
}


size_t FrameProtocol::remainingBufferSize() {
	if(isReadComplete()) {
		//printf("===========> FrameProtocol::remainingBufferSize : %d %d %d %d %d\n", readBuffer_->remainingSize(), initHeader_.length(), payload_pos_, readPayloadSize(), readBuffer_->remainingSize() - (initHeader_.length() - payload_pos_ - readPayloadSize()));
		return readBuffer_->remainingSize() - (initHeader_.length() - payload_pos_ - readPayloadSize());
	}
	return BaseProtocol::remainingBufferSize();
}


const void * FrameProtocol::payloadPtr() {
	// for performance, do not copy to payload_ string.
	if(payload_.size() > 0)
		return payload_.c_str();
	else if(isReadComplete()) {
		if(payload_pos_ < payloadBuffer()->totalSize())
			return (const char*)payloadBuffer()->basePtr() + payload_pos_;
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

