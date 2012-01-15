#pragma once

#include <string>
#include <vector>
#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#endif
#include "BaseVirtualTransport.h"
#include "Exception.h"
#if defined(WIN32)
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace coconut { 

class COCONUT_API VirtualTransportHelper {
public:
	static boost::int32_t writeInt32(boost::shared_ptr<BaseVirtualTransport> buffer, boost::int32_t value) {
		boost::int32_t net = (boost::int32_t)htonl(value);
		buffer->write((boost::int8_t*)&net, 4);
		return 4;
	}
	
	static boost::int32_t writeBinary(boost::shared_ptr<BaseVirtualTransport> buffer, const void *data, size_t size) {
		boost::int32_t pos = 0;
		pos += buffer->write(data, size);
		return pos;
	}

	static boost::int32_t writeString(boost::shared_ptr<BaseVirtualTransport> buffer, const std::string &value) {
		boost::int32_t pos = 0;
		pos += writeInt32(buffer, value.size());
		pos += buffer->write(value.c_str(), value.size());
		return pos;
	}

	static boost::int32_t writeStringList(boost::shared_ptr<BaseVirtualTransport> buffer, const stringlist_t &list) {
		boost::int32_t pos = 0;
		pos += writeInt32(buffer, (boost::int32_t)list.size());
		for(size_t i = 0; i < list.size(); i++) {
			pos += writeString(buffer, list[i]);
		}
		return pos;
	}

	static boost::int32_t readInt32(boost::shared_ptr<BaseVirtualTransport> buffer, boost::int32_t &value) {
		union bytes {
			boost::int8_t b[4];
			boost::int32_t all;
		} theBytes;

		if(buffer->read((void *)theBytes.b, 4) != 4)
			throw ProtocolException("readInt32 failed..");
		else
			value = (boost::int16_t)ntohl(theBytes.all);
		return 4;
	}

	static boost::int32_t readInt8(boost::shared_ptr<BaseVirtualTransport> buffer, boost::int8_t &value) {
		boost::int8_t b[1];
		int res = buffer->read((void *)b, 1);
		if(res != 1)
			throw ProtocolException("readInt8 failed..");
		else
			value = b[0];
		return 1;
	}

	static boost::int32_t readStringFast(boost::shared_ptr<BaseVirtualTransport> buffer, std::string &value) {
		boost::int32_t len = 0;
		boost::int32_t pos = 0;
		pos += readInt32(buffer, len);
		int nread = buffer->read(value, len);
		if(nread != len)
			throw ProtocolException("readStringFast failed..");
		return pos + len;
	}
};

}
