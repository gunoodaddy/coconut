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
#include "FrameController.h"

using namespace coconut::protocol;

namespace coconut {
	
void FrameController::writeFrame(const protocol::FrameHeader &header, boost::shared_ptr<BufferedTransport> buffer) {
	writeFrame(header, buffer->currentPtr(), buffer->remainingSize());
}

/*
static bool hasParentFrameProtocol(BaseProtocol *protocol) {
	BaseProtocol *parent = protocol->parentProtocol();
	while(parent) {
		if(strcmp(parent->className(), "FrameProtocol") == 0) {
			return true;
		}
		parent = parent->parentProtocol();
	}
	boost::shared_ptr<BaseProtocol> parent_share_ptr = protocol->parentProtocolSharedPtr();
	while(parent_share_ptr) {
		if(strcmp(parent_share_ptr->className(), "FrameProtocol") == 0) {
			return true;
		}
		parent_share_ptr = parent_share_ptr->parentProtocolSharedPtr();
	}
	return false;
}
*/


static bool disableWriteBufferOfParentFrameProtocol(BaseProtocol *protocol) {
	BaseProtocol *parent = protocol->parentProtocol();
	while(parent) {
		if(strcmp(parent->className(), "FrameProtocol") == 0) {
			parent->enableWriteBuffer(false);
			return true;
		}
		parent = parent->parentProtocol();
	}
	boost::shared_ptr<BaseProtocol> parent_share_ptr = protocol->parentProtocolSharedPtr();
	while(parent_share_ptr) {
		if(strcmp(parent_share_ptr->className(), "FrameProtocol") == 0) {
			parent_share_ptr->enableWriteBuffer(false);
			return true;
		}
		parent_share_ptr = parent_share_ptr->parentProtocolSharedPtr();
	}
	return false;
}


void FrameController::writeFrame(const protocol::FrameHeader &header, boost::shared_ptr<BaseProtocol> protocol) {
	
	//assert(!hasParentFrameProtocol(protocol.get()) && "Protocol must not have parent FrameProtocol.");
	disableWriteBufferOfParentFrameProtocol(protocol.get());

	boost::shared_ptr<FrameProtocol> prot 
		= boost::static_pointer_cast<FrameProtocol>(protocolFactory_->makeProtocol());

	prot->setFrame(header, protocol);
	prot->processSerialize();
	prot->processWrite(socket());
}

void FrameController::writeFrame(const protocol::FrameHeader &header, BaseProtocol *protocol) {
	//assert(!hasParentFrameProtocol(protocol) && "Protocol must not have parent FrameProtocol.");
	disableWriteBufferOfParentFrameProtocol(protocol);

	boost::shared_ptr<FrameProtocol> prot 
		= boost::static_pointer_cast<FrameProtocol>(protocolFactory_->makeProtocol());

	prot->setFrame(header, protocol);
	prot->processSerialize();
	prot->processWrite(socket());
}

void FrameController::writeFrame(const FrameHeader &header, const void *payload, size_t size) {
	boost::shared_ptr<FrameProtocol> prot 
		= boost::static_pointer_cast<FrameProtocol>(protocolFactory_->makeProtocol());

	prot->setFrame(header, payload, size);
	prot->processSerialize();
	prot->processWrite(socket());
}

void FrameController::onReceivedProtocol(boost::shared_ptr<protocol::BaseProtocol> protocol) {
	boost::shared_ptr<FrameProtocol> frame = boost::static_pointer_cast<FrameProtocol>(protocol);
	_LOG_TRACE("FrameController received : %d", frame->header().command());
	onFrameReceived(frame);
}

} // end of namespace coconut
