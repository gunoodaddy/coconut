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
#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#endif
#include "BaseProtocol.h"
#include "BaseObjectAllocator.h"

namespace coconut { namespace protocol {

class COCONUT_API LineProtocol 
				: public ProtocolDecorator
				, public BaseObjectAllocator<LineProtocol>
{
public:
	LineProtocol();
	LineProtocol(BaseProtocol *protocol);
	LineProtocol(boost::shared_ptr<BaseProtocol> protocol);
	~LineProtocol();

	const char* className() {
		return "LineProtocol";
	}

	bool isReadComplete() {
		return readComplete_;
	}

	bool processRead(boost::shared_ptr<BaseVirtualTransport> transport);

	bool processSerialize(size_t bufferSize = 0);

	const void * remainingBufferPtr();
	size_t remainingBufferSize();

public:
	void setLine(const std::string &string) {
		line_ = string;
	}

	const char* linePtr() {
		return line_.c_str();
	}

private:
	std::string line_;
	volatile bool readComplete_;
	int remainReadBufferSize_;
};

} }

