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

#include "Coconut.h"
#include "Exception.h"
#include "InternalLogger.h"
#include "BufferedTransport.h"

namespace coconut { 
BufferedTransport::BufferedTransport() : readPos_(0) { 
	_LOG_TRACE("BufferedTransport() : %p", this);
}

BufferedTransport::~BufferedTransport() {
	_LOG_TRACE("~BufferedTransport() : %p", this);
}

int BufferedTransport::write(const char* ptr, size_t size) {
	return write((const void *)ptr, size);
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

int BufferedTransport::peek(char *buffer, size_t size) {
	ScopedMutexLock(lock_);
	if(remainingSize() < size) {
		size = remainingSize();
	}
	memcpy(buffer, currentPtr(), size);
	return size;
}

const void * BufferedTransport::peek(size_t &size) {
	ScopedMutexLock(lock_);
	if(remainingSize() < size) {
		size = remainingSize();
	}
	return currentPtr();
}

void BufferedTransport::throwAway(size_t size) {
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

void BufferedTransport::setReadPos(size_t pos) {
	ScopedMutexLock(lock_);
	if(pos >= totalSize())
		throw ProtocolException("readpos less than 0 by rewind");

	readPos_ = pos;
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
		throw Exception("readpos exceed buffer size by fastforward");
	readPos_ = temp;
}


}

