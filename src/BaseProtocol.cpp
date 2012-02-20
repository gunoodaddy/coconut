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
#include "InternalLogger.h"
#include "BaseProtocol.h"
#include <VirtualTransportHelper.h>

namespace coconut { namespace protocol {

bool BaseProtocol::processWriteFromReadBuffer(boost::shared_ptr<BaseVirtualTransport> transport) {

	if(parent_protocol_) {
		if(false == parent_protocol_->processWriteFromReadBuffer(transport))
			return false;
	} else if(parent_protocol_shared_ptr_) {
		if(false == parent_protocol_shared_ptr_->processWriteFromReadBuffer(transport))
			return false;
	}

	_LOG_TRACE("BaseProtocol::processWrite %d %s : %d\n", turnOnWrite_, className(), readBuffer_->remainingSize());

	if(turnOnWrite_ && readBuffer_->remainingSize() > 0) {
		boost::int32_t pos = transport->write(readBuffer_->currentPtr(), readBuffer_->remainingSize());
		if(pos != (int)readBuffer_->remainingSize()) {
			throw ProtocolException("processWrite not write all buffer to transport");
		} else {
			readBuffer_->fastforward(pos);
		}
	}
	return true;
}


bool BaseProtocol::processWrite(boost::shared_ptr<BaseVirtualTransport> transport) {

	if(parent_protocol_) {
		if(false == parent_protocol_->processWrite(transport))
			return false;
	} else if(parent_protocol_shared_ptr_) {
		if(false == parent_protocol_shared_ptr_->processWrite(transport))
			return false;
	}

	_LOG_TRACE("BaseProtocol::processWrite %d %s : %d\n", turnOnWrite_, className(), writebuffer_->remainingSize());

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
		payload = readBuffer_;
	}
	return payload;
}

void BaseProtocol::resetReadingBufferToRemainingBuffer() {

	std::string rest;
	rest.assign((const char*)remainingBufferPtr(), remainingBufferSize());

	resetBuffer();
	addToReadingBuffer(rest.c_str(), rest.size());
}
	
} }

