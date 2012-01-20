#include "Coconut.h"
#include "Exception.h"
#include "Logger.h"
#include "BufferedTransport.h"

namespace coconut { 
BufferedTransport::BufferedTransport() : readPos_(0) { 
	LOG_TRACE("BufferedTransport : %p", this);
}

BufferedTransport::~BufferedTransport() {
	LOG_TRACE("~BufferedTransport : %p", this);
}

int BufferedTransport::write(const void* ptr, size_t size) {
	ScopedMutexLock(lock_);
	buffer_.append((const char*)ptr, size);
	return size;
}

int BufferedTransport::read(std::string &data, size_t size) {
	ScopedMutexLock(lock_);
	if(remainingSize() < size) {
		size = remainingSize();
	}
	data.assign((const char *)currentPtr(), size);
	fastforward(size);
	return size;
}

int BufferedTransport::read(void *ptr, size_t size) {
	ScopedMutexLock(lock_);
	if(remainingSize() < size) {
		size = remainingSize();
	}
	memcpy(ptr, currentPtr(), size);
	fastforward(size);
	return size;
}

const void * BufferedTransport::peek(size_t &size) {
	ScopedMutexLock(lock_);
	if(remainingSize() < size) {
		size = remainingSize();
	}
	return currentPtr();
}

void BufferedTransport::ackReadSize(size_t size) {
	fastforward(size);
}

size_t BufferedTransport::totalSize() {
	return buffer_.size();
}

size_t BufferedTransport::remainingSize() {
	return buffer_.size() - readPos_;
}

const void * BufferedTransport::basePtr() {
	return buffer_.c_str();
}

const void * BufferedTransport::currentPtr() {
	return buffer_.c_str() + readPos_;
}

size_t BufferedTransport::readPos() {
	return readPos_;
}

void BufferedTransport::clear() {
	ScopedMutexLock(lock_);
	readPos_ = 0;
	buffer_.clear();
}

void BufferedTransport::rewind(size_t size) {
	ScopedMutexLock(lock_);
	int temp = readPos_ - size;
	if(temp < 0) {
		throw ProtocolException("readpos less than 0 by rewind");
	}
	readPos_ = temp;
}

void BufferedTransport::fastforward(size_t size) {
	ScopedMutexLock(lock_);
	int temp = readPos_ + size;
	if(temp > (int)buffer_.size())
		throw ProtocolException("readpos exceed buffer size by fastforward");
	readPos_ = temp;
}


}

