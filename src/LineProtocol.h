#pragma once

#include <string>
#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#endif
#include "BaseProtocol.h"
#include "BaseVirtualTransport.h"
#include "BufferedTransport.h"
#include "Exception.h"
#include "Logger.h"

namespace coconut { namespace protocol {

class COCONUT_API LineProtocol : public ProtocolDecorator {
public:
	LineProtocol() : readComplete_(false) {
		LOG_TRACE("LineProtocol : %p", this);
	}

	LineProtocol(BaseProtocol *protocol) : readComplete_(false) {
		LOG_TRACE("LineProtocol with parent_protocol : %p", this);
		parent_protocol_ = protocol;
	}
	LineProtocol(boost::shared_ptr<BaseProtocol> protocol) : readComplete_(false) {
		LOG_TRACE("LineProtocol with parent_protocol_shared_ptr : %p", this);
		parent_protocol_shared_ptr_ = protocol;
	}


	~LineProtocol() {
		LOG_TRACE("~LineProtocol : %p", this);
	}

	const char* className() {
		return "LineProtocol";
	}

	bool isReadComplete() {
		return readComplete_;
	}

	bool processRead(boost::shared_ptr<BaseVirtualTransport> transport);

	bool processSerialize(size_t bufferSize = 0);

public:
	void setLine(const std::string &string) {
		line_ = string;
	}

	const char* linePtr() {
		return line_.c_str();
	}

private:
	std::string line_;
	bool readComplete_;
};

} }

