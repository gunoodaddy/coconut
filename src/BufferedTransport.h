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
#include "BaseVirtualTransport.h"
#include "ThreadUtil.h"
#include "Exception.h"
#if defined(WIN32)
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace coconut { 

class COCONUT_API BufferedTransport : public BaseVirtualTransport {
public:
	BufferedTransport();
	~BufferedTransport();

public:
	const char *className() {
		return "BufferedTransport";
	}
	
	int erase(size_t pos, size_t size);
	int write(const char* ptr, size_t size);
	int write(const void* ptr, size_t size);
	int read(std::string &data, size_t size);
	int read(void *ptr, size_t size);
	const void * peek(size_t &size);
	int peek(char *buffer, size_t size);
	void throwAway(size_t size);
	int insertInt8(size_t pos, boost::int8_t value);

	boost::int32_t writeInt32(boost::int32_t value) {
		boost::int32_t net;
		if(_setLittleEndian_on) {
			net = (boost::int32_t)value;
		} else {
			net = (boost::int32_t)htonl(value);
		}
		write((boost::int8_t*)&net, 4);
		return 4;
	}
	
	boost::int16_t writeInt16(boost::int16_t value) {
		boost::int16_t net;
		if(_setLittleEndian_on) {
			net = (boost::int16_t)value;
		} else {
			net = (boost::int16_t)htons(value);
		}
		write((boost::int8_t*)&net, 2);
		return 2;
	}
	
	boost::int8_t writeInt8(boost::int8_t value) {
		write((boost::int8_t*)&value, 1);
		return 1;
	}
	
	boost::int32_t writeBinary(const void *data, size_t size) {
		boost::int32_t pos = 0;
		pos += write(data, size);
		return pos;
	}

	boost::int32_t writeString8(const std::string &value) {
		boost::int8_t pos = 0;
		pos += writeInt8(value.size());
		pos += write(value.c_str(), value.size());
		return pos;
	}

	boost::int32_t writeString16(const std::string &value) {
		boost::int16_t pos = 0;
		pos += writeInt16(value.size());
		pos += write(value.c_str(), value.size());
		return pos;
	}

	boost::int32_t writeString32(const std::string &value) {
		boost::int32_t pos = 0;
		pos += writeInt32(value.size());
		pos += write(value.c_str(), value.size());
		return pos;
	}

	boost::int32_t writeString32List(const stringlist_t &list) {
		boost::int32_t pos = 0;
		pos += writeInt32((boost::int32_t)list.size());
		for(size_t i = 0; i < list.size(); i++) {
			pos += writeString32(list[i]);
		}
		return pos;
	}

	// Reading helper method
public:
	boost::int32_t readInt32(boost::int32_t &value) {
		union bytes {
			boost::int8_t b[4];
			boost::int32_t all;
		} theBytes;

		if(read((void *)theBytes.b, 4) != 4)
			throw ProtocolException("readInt32 failed..");
		else {
			if(_setLittleEndian_on)
				value = (boost::int32_t)theBytes.all;
			else
				value = (boost::int32_t)ntohl(theBytes.all);
		}
		return 4;
	}

	boost::int32_t readInt32() {
		union bytes {
			boost::int8_t b[4];
			boost::int32_t all;
		} theBytes;

		if(read((void *)theBytes.b, 4) != 4)
			throw ProtocolException("readInt32 failed..");
		if(_setLittleEndian_on) {
			return (boost::int32_t)theBytes.all;
		} else {
			return (boost::int32_t)ntohl(theBytes.all);
		}
	}

	boost::int16_t readInt16(boost::int16_t &value) {
		union bytes {
			boost::int8_t b[2];
			boost::int16_t all;
		} theBytes;

		if(read((void *)theBytes.b, 2) != 2)
			throw ProtocolException("readInt16 failed..");
		else {
			if(_setLittleEndian_on)
				value = (boost::int16_t)theBytes.all;
			else
				value = (boost::int16_t)ntohs(theBytes.all);
		}
		return 2;
	}

	boost::int16_t readInt16() {
		union bytes {
			boost::int8_t b[2];
			boost::int16_t all;
		} theBytes;

		if(read((void *)theBytes.b, 2) != 2)
			throw ProtocolException("readInt16 failed..");
		if(_setLittleEndian_on) {
			return (boost::int16_t)theBytes.all;
		} else {
			return (boost::int16_t)ntohs(theBytes.all);
		}
	}

	boost::int8_t readInt8(boost::int8_t &value) {
		boost::int8_t b[1];
		int res = read((void *)b, 1);
		if(res != 1)
			throw ProtocolException("readInt8 failed..");
		else
			value = b[0];
		return 1;
	}

	boost::int8_t readInt8() {
		boost::int8_t b[1];
		int res = read((void *)b, 1);
		if(res != 1)
			throw ProtocolException("readInt8 failed..");
		return b[0];
	}

	boost::int32_t readString32(std::string &value) {
		boost::int8_t len = 0;
		boost::int8_t pos = 0;
		pos += readInt8(len);
		int nread = read(value, len);
		if(nread != len)
			throw ProtocolException("readString32 failed..");
		return pos + len;
	}

	std::string readString32() {
		std::string str;
		size_t len = readInt8();
		int nread = read(str, len);
		if(nread != (int)len)
			throw ProtocolException("readString32 failed..");
		return str;
	}

	boost::int32_t readString16(std::string &value) {
		boost::int16_t len = 0;
		boost::int16_t pos = 0;
		pos += readInt16(len);
		int nread = read(value, len);
		if(nread != len)
			throw ProtocolException("readString16 failed..");
		return pos + len;
	}

	std::string readString16() {
		std::string str;
		size_t len = readInt16();
		int nread = read(str, len);
		if(nread != (int)len)
			throw ProtocolException("readString16 failed..");
		return str;
	}

	boost::int32_t readString8(std::string &value) {
		boost::int32_t len = 0;
		boost::int32_t pos = 0;
		pos += readInt32(len);
		int nread = read(value, len);
		if(nread != len)
			throw ProtocolException("readString8 failed..");
		return pos + len;
	}

	std::string readString8() {
		std::string str;
		size_t len = readInt8();
		int nread = read(str, len);
		if(nread != (int)len)
			throw ProtocolException("readString8 failed..");
		return str;
	}
	
public:
	size_t totalSize();
	size_t remainingSize();
	size_t readPos();
	void setReadPos(size_t pos);

	const void * basePtr();
	const void * currentPtr();

	void clear();
	void rewind(size_t size);
	void fastforward(size_t size);

private:
	std::string buffer_;
	int readPos_;
	/*int writePos_;*/	// write position is not necessary by using std::string
	Mutex lock_;
};

}

