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
#include "StringListProtocol.h"
#include <VirtualTransportHelper.h>

namespace coconut { namespace protocol {

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
						payload_pos_ += VirtualTransportHelper::readString32(transport, string);
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
	VirtualTransportHelper::writeString32List(writebuffer_, list_);

	return callParentProcessSerialize(writebuffer_->totalSize() + bufferSize);
}

	
} }

