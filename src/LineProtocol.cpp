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
#include "LineProtocol.h"
#include "VirtualTransportHelper.h"
#include "InternalLogger.h"

namespace coconut { namespace protocol {

LineProtocol::LineProtocol() : readComplete_(false), remainReadBufferSize_(0) {
	_LOG_TRACE("LineProtocol : %p", this);
}

LineProtocol::LineProtocol(BaseProtocol *protocol) : readComplete_(false), remainReadBufferSize_(0) {
	_LOG_TRACE("LineProtocol with parent_protocol : %p", this);
	parent_protocol_ = protocol;
}

LineProtocol::LineProtocol(boost::shared_ptr<BaseProtocol> protocol) : readComplete_(false), remainReadBufferSize_(0) {
	_LOG_TRACE("LineProtocol with parent_protocol_shared_ptr : %p", this);
	parent_protocol_shared_ptr_ = protocol;
}

LineProtocol::~LineProtocol() {
	_LOG_TRACE("~LineProtocol : %p", this);
}

bool LineProtocol::processSerialize(size_t bufferSize) {
	resetWritingBuffer();
	VirtualTransportHelper::writeBinary(writebuffer_, line_.c_str(), line_.size());
	VirtualTransportHelper::writeBinary(writebuffer_, "\r\n", 2);

	if(callParentProcessSerialize(bufferSize + writebuffer_->totalSize()) == false)
		return false;

	return true;
}

#define BUFFER_SIZE 4096

bool LineProtocol::processRead(boost::shared_ptr<BaseVirtualTransport> transport) {
	if(callParentProcessRead(transport) == false)
		return false;

	try {
		char buffer[BUFFER_SIZE] = {0, };
		int nread = transport->read(buffer, BUFFER_SIZE);	
		
		for(int i = 0; i < nread; i++) {
			if(buffer[i] == '\n') {
				size_t addition = 0;
				size_t delimSize = 1;
				if(buffer[i-1] != '\r') {	// \r\n check
					if(i < nread - 1) {
						if( buffer[i + 1] == '\r' )	{ // \n\r check
							addition = 1;
							delimSize = 2;
						}
					}
				} else {
					delimSize = 2;
				}
				remainReadBufferSize_ = nread - i - (1 + addition);
				line_.append(buffer, i - (delimSize - 1));
				_LOG_DEBUG("LineProtocol line received : [%s]", line_.c_str()); 
				readComplete_ = true;
				return true;
			}
		}
	} catch (SocketException &e) {
		(void)e;
	} catch (ProtocolException &e) {
		(void)e;
	}
	return false;
}

const void * LineProtocol::remainingBufferPtr() {
	if(isReadComplete()) {
		return (const char*)payloadBuffer()->currentPtr() - remainReadBufferSize_;
	}
	return BaseProtocol::remainingBufferPtr();
}

size_t LineProtocol::remainingBufferSize() {
	if(isReadComplete()) {
		return remainReadBufferSize_;
	}
	return BaseProtocol::remainingBufferSize();
}

}}

