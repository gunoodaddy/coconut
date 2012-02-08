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

#include "config.h"
#include <string>
#include <vector>
#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#endif
#include "BaseVirtualTransport.h"
#include "BufferedTransport.h"
#include "Exception.h"

namespace coconut { namespace protocol {

class COCONUT_API BaseProtocol {
public:
	BaseProtocol() : parent_protocol_(NULL), turnOnWrite_(true) {
		readBuffer_ = boost::shared_ptr<BufferedTransport>(new BufferedTransport);
		writebuffer_ = boost::shared_ptr<BufferedTransport>(new BufferedTransport);
	}
	virtual ~BaseProtocol() { 
	}

	virtual const char* className() = 0;
	virtual bool isReadComplete() = 0;
	virtual bool processRead(boost::shared_ptr<BaseVirtualTransport> transport) = 0;
	virtual bool processWrite(boost::shared_ptr<BaseVirtualTransport> transport);
	virtual bool processWriteFromReadBuffer(boost::shared_ptr<BaseVirtualTransport> transport);
	virtual bool processSerialize(size_t bufferSize = 0) = 0;

	virtual const void * remainingBufferPtr() {
		return readBuffer_->currentPtr();
	}

	virtual size_t remainingBufferSize() {
		return readBuffer_->remainingSize();
	}

	virtual bool isInvalidPacketReceived() {
		return false;
	}

	size_t writingBufferSize() {
		if(parent_protocol_ && parent_protocol_->isEnabledWriteBuffer()) {
			return writebuffer_->totalSize() + parent_protocol_->writingBufferSize();
		} else if(parent_protocol_shared_ptr_ && parent_protocol_shared_ptr_->isEnabledWriteBuffer()) {
			return writebuffer_->totalSize() + parent_protocol_shared_ptr_->writingBufferSize();
		}
		return  writebuffer_->totalSize();
	}

	bool callParentProcessRead(boost::shared_ptr<BaseVirtualTransport> transport) {
		if(parent_protocol_ && parent_protocol_->isReadComplete() == false) {
			if(parent_protocol_->processRead(transport) == false) 
				return false;
		} else if(parent_protocol_shared_ptr_ && parent_protocol_shared_ptr_->isReadComplete() == false) {
			if(parent_protocol_shared_ptr_->processRead(transport) == false) 
				return false;
		}
		return true;
	}

	bool callParentProcessSerialize(size_t bufferSize) {
		if(parent_protocol_)
			return parent_protocol_->processSerialize(bufferSize);
		else if(parent_protocol_shared_ptr_)
			return parent_protocol_shared_ptr_->processSerialize(bufferSize);
		return true;
	}

	bool processReadFromReadingBuffer() {
		return processRead(readBuffer_);
	}

	bool processReadFromPayloadBuffer() {
		return processRead(payloadBuffer());
	}

	void addToReadingBuffer(const void *data, size_t size) {
		readBuffer_->write(data, size);
	}

	virtual boost::shared_ptr<BufferedTransport> payloadBuffer();	// or readBuffer()
	boost::shared_ptr<BufferedTransport> writingBuffer() {
		return writebuffer_;
	}

	void resetReadingBufferToRemainingBuffer();
	void resetBuffer() {
		readBuffer_->clear();
		resetWritingBuffer();
	}
	void resetWritingBuffer() {
		writebuffer_->clear();
	}

	void enableWriteBuffer(bool enable) {
		turnOnWrite_ = enable;
	}

	BaseProtocol *parentProtocol() {
		return parent_protocol_;
	}

	boost::shared_ptr<BaseProtocol> parentProtocolSharedPtr() {
		return parent_protocol_shared_ptr_;
	}

	bool isEnabledWriteBuffer() {
		return turnOnWrite_;
	}

protected:
	boost::shared_ptr<BufferedTransport> readBuffer_;
	boost::shared_ptr<BufferedTransport> writebuffer_;
	BaseProtocol *parent_protocol_;
	boost::shared_ptr<BaseProtocol> parent_protocol_shared_ptr_;
	bool turnOnWrite_;
};


class COCONUT_API ProtocolDecorator : public BaseProtocol {
public:
	ProtocolDecorator() {}
	virtual ~ProtocolDecorator() {}
};


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
	boost::int32_t listSize_;
};

class COCONUT_API BaseProtocolFactory {
public:
	virtual ~BaseProtocolFactory() { }

	virtual boost::shared_ptr<BaseProtocol> makeProtocol() = 0;
};

} }

