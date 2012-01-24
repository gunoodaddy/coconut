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
#include "InternalLogger.h"
#include "FileDescriptorController.h"

using namespace coconut::protocol;

namespace coconut {

void FileDescriptorController::writeDescriptor(int fd) {
	if(protocolFactory_) {
		boost::shared_ptr<FileDescriptorProtocol> fdProtocol 
			= boost::static_pointer_cast<FileDescriptorProtocol>(protocolFactory_->makeProtocol());

		fdProtocol->setFileDescriptor(fd);
		fdProtocol->processSerialize();
		fdProtocol->processWrite(socket());
	}
}

void FileDescriptorController::onSocket_ReadEvent(int fd) { 
	if(protocolFactory_) {
		do {
			if(!protocol_ || protocol_->isReadComplete()) {
				_LOG_TRACE("New Protocol make #1 in %p\n", this);
				protocol_ = protocolFactory_->makeProtocol();
			}

			if(protocol_->processRead(socket()) == true) {
				onReceivedProtocol(protocol_);
			} else {
				break;
			}
		} while(1);
	} else {
		char buffer[IOBUF_LEN];
		int res = socket()->read(buffer, IOBUF_LEN);
		if(res > 0)
			onReceivedData(buffer, res);
	}
}


void FileDescriptorController::onReceivedProtocol(boost::shared_ptr<protocol::BaseProtocol> protocol) {
	boost::shared_ptr<FileDescriptorProtocol> fdProtocol = boost::static_pointer_cast<FileDescriptorProtocol>(protocol);
	onDescriptorReceived(fdProtocol->fileDescriptor());
}

} // end of namespace coconut
