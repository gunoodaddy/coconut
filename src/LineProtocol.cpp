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

