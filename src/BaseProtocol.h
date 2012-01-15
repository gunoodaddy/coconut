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
		buffer_ = boost::shared_ptr<BufferedTransport>(new BufferedTransport);
		writebuffer_ = boost::shared_ptr<BufferedTransport>(new BufferedTransport);
	}
	virtual ~BaseProtocol() { 
	}

	virtual const char* className() = 0;
	virtual bool isReadComplete() = 0;
	virtual bool processRead(boost::shared_ptr<BaseVirtualTransport> transport) = 0;
	virtual bool processWrite(boost::shared_ptr<BaseVirtualTransport> transport);
	virtual bool processSerialize(size_t bufferSize = 0) = 0;

	virtual const void * remainingBufferPtr() {
		return buffer_->currentPtr();
	}
	virtual size_t remainingBufferSize() {
		return buffer_->remainingSize();
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
		return processRead(buffer_);
	}

	bool processReadFromPayloadBuffer() {
		return processRead(payloadBuffer());
	}

	void addToReadingBuffer(const void *data, size_t size) {
		buffer_->write(data, size);
	}

	boost::shared_ptr<BufferedTransport> payloadBuffer();	// or readBuffer()
	boost::shared_ptr<BufferedTransport> writingBuffer() {
		return writebuffer_;
	}

	void resetReadingBufferToRemainingBuffer();
	void resetBuffer() {
		buffer_->clear();
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
	boost::shared_ptr<BufferedTransport> buffer_;
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


/*
class COCONUT_API JSONProtocol : public ProtocolDecorator {
public:
	JSONProtocol(BaseProtocol *protocol) {
		parent_protocol_ = protocol;
	}
	JSONProtocol(boost::shared_ptr<BaseProtocol> protocol) {
		parent_protocol_shared_ptr_ = protocol;
	}

	bool isReadComplete() {
		return false;
	}

	bool processRead(boost::shared_ptr<BaseVirtualTransport> transport) {
		if(parent_protocol_->processRead(transport) == false) 
			return false;

		// TODO	
		return true;
	}

	bool processSerialize(size_t bufferSize = 0) {
		// TODO

		return parent_protocol_->processSerialize(writebuffer_->totalSize() + bufferSize);
	}

private:
	std::string json_; // TODO
};
*/


class COCONUT_API BaseProtocolFactory {
public:
	virtual ~BaseProtocolFactory() { }

	virtual boost::shared_ptr<BaseProtocol> makeProtocol() = 0;
};

} }

