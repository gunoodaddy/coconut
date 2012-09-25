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

#include "BaseProtocol.h"

namespace coconut { namespace protocol {

class COCONUT_API StringListProtocol : public ProtocolDecorator {
public:
	enum State{
		Init,
		Begin,
		Contents,
		End
	};

	StringListProtocol() : state_(Init), payload_pos_(0), listSize_(0) {}
	StringListProtocol(BaseProtocol *protocol) : state_(Init), payload_pos_(0), listSize_(0) {
		parent_protocol_ = protocol;
	}
	StringListProtocol(boost::shared_ptr<BaseProtocol> protocol) : state_(Init), payload_pos_(0), listSize_(0) {
		parent_protocol_shared_ptr_ = protocol;
	}

	const char* className() {
		return "StringListProtocol";
	}
	bool isReadComplete() {
		return state_ == End;
	}

	bool processRead(boost::shared_ptr<BaseVirtualTransport> transport);
	bool processSerialize(size_t bufferSize = 0);

public:
	void addString(std::string str) {
		list_.push_back(str);
	}

	const std::string& stringOf(size_t index) {
		return list_[index];
	}

	size_t listSize() {
		return list_.size();
	}

private:
	stringlist_t list_;
	State state_;
	int payload_pos_;
	boost::uint32_t listSize_;
};

} }

