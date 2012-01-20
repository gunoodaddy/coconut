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

class JSONProtocolImpl;

class COCONUT_API JSONProtocol : public ProtocolDecorator {
public:
	JSONProtocol();
	JSONProtocol(BaseProtocol *protocol);
	JSONProtocol(boost::shared_ptr<BaseProtocol> protocol);
	~JSONProtocol();

	const char* className() {
		return "JSONProtocol";
	}

	bool isReadComplete();
	bool processRead(boost::shared_ptr<BaseVirtualTransport> transport);
	bool processSerialize(size_t bufferSize = 0);
	const void * remainingBufferPtr();
	size_t remainingBufferSize();

public:
	void setJSON(const std::string &jsonString);
	const char* jsonPtr();

private:
	JSONProtocolImpl *impl_;
};

} }

