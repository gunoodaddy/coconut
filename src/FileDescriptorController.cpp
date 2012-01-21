#include "Coconut.h"
#include "Logger.h"
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
				LOG_TRACE("New Protocol make #1 in %p\n", this);
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
