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
#endif
#include "BaseVirtualTransport.h"
#include "VirtualTransportHelper.h"
#include "BufferedTransport.h"
#include "BaseProtocol.h"
#include "BaseObjectAllocator.h"

namespace coconut { namespace protocol {

class COCONUT_API FrameHeader {
public:
	FrameHeader(boost::uint32_t command, boost::uint32_t transactionId) : length_(0), checksum_(0), cmd_(command), trid_(transactionId) { }
	FrameHeader() : length_(0), checksum_(0), cmd_(0), trid_(0) { }

	void setCommand(boost::uint32_t command) {
		cmd_ = command;	
	}
	boost::uint32_t command() const {
		return cmd_;
	}

	void setChecksum(boost::uint32_t checksum) {
		checksum_ = checksum;	
	}
	boost::uint32_t checksum() const {
		return checksum_;
	}

	void setLength(boost::uint32_t length) {
		length_ = length;	
	}
	boost::uint32_t length() const {
		return length_;
	}

	void setTransactionId(boost::uint32_t trid) {
		trid_ = trid;	
	}
	boost::uint32_t transactionId() const {
		return trid_;
	}

	void makeChecksum(const void *payload, size_t size) {
		// TODO check sum logic need..
		checksum_ = 1;
	}

	void serialize(boost::shared_ptr<BufferedTransport> buffer, size_t payloadSize) {
		VirtualTransportHelper::writeInt32(buffer, cmd_);
		VirtualTransportHelper::writeInt32(buffer, trid_);
		VirtualTransportHelper::writeInt32(buffer, checksum_);
		// auto calculated..
		length_ = buffer->totalSize() + payloadSize + sizeof(length_);
		VirtualTransportHelper::writeInt32(buffer, length_);
	}

private:
	boost::uint32_t length_;	// auto calculated
	boost::uint32_t checksum_;	// auto calculated
	boost::uint32_t cmd_;		// user definition
	boost::uint32_t trid_;		// user definition
};


class COCONUT_API FrameProtocol 
				: public BaseProtocol
				, public BaseObjectAllocator<FrameProtocol>
{
public:
	enum State{
		Init,
		Command,
		TransationID,
		Checksum,
		Length,
		Payload,
		Complete
	};

	FrameProtocol() : header_(), initHeader_(), state_(Init), payload_pos_(0), payload_protocol_(NULL) { }

	~FrameProtocol() { 
	}

	const char* className() {
		return "FrameProtocol";
	}

	bool isReadComplete() {
		return state_ == Complete;
	}

	bool processSerialize(size_t bufferSize = 0);
	bool processRead(boost::shared_ptr<BaseVirtualTransport> transport);
	bool processWrite(boost::shared_ptr<BaseVirtualTransport> transport);

	size_t readPayloadSize() {
		return readBuffer_->readPos() - payload_pos_;
	}
	virtual const void * remainingBufferPtr();
	virtual size_t remainingBufferSize();

public:
	const FrameHeader &header() {
		return header_;
	}

	const void * payloadPtr();
	size_t payloadSize();

	void setFrame(const FrameHeader &header, BaseProtocol *protocol) {
		header_ = header;
		payload_protocol_ = protocol;

		state_ = Complete;
	}

	void setFrame(const FrameHeader &header, boost::shared_ptr<BaseProtocol> protocol) {
		header_ = header;
		payload_protocol_shared_ptr_ = protocol;

		state_ = Complete;
	}

	void setFrame(const FrameHeader &header, const void *payload, size_t size) {
		header_ = header;
		payload_.assign((char *)payload, size);

		state_ = Complete;
	}

private:
	FrameHeader header_;
	FrameHeader initHeader_;
	State state_;
	std::string payload_;
	size_t payload_pos_;
	BaseProtocol *payload_protocol_;
	boost::shared_ptr<BaseProtocol> payload_protocol_shared_ptr_;
};

} }

