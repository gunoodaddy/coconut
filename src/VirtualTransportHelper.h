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
