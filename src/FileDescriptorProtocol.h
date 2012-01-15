#pragma once

#include <string>
#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#endif
#include "BaseProtocol.h"
#include "BaseVirtualTransport.h"
#include "Exception.h"

namespace coconut { namespace protocol {

class COCONUT_API FileDescriptorProtocol : public ProtocolDecorator {
public:
	FileDescriptorProtocol() : fd_(0), readComplete_(false) { }

	const char* className() {
		return "FileDescriptorProtocol";
	}

	bool isReadComplete() {
		return readComplete_;
	}

	bool processRead(boost::shared_ptr<BaseVirtualTransport> transport);
	bool processWrite(boost::shared_ptr<BaseVirtualTransport> transport);

	bool processSerialize(size_t bufferSize = 0) {
		resetBuffer();
		// nothing to do..
		return true;
	}

public:
	void setFileDescriptor(int fd) {
		fd_ = fd;
	}

	int fileDescriptor() {
		return fd_;
	}

private:
	int fd_;
	bool readComplete_;
};

} }

