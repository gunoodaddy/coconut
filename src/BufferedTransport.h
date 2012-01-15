#pragma once

#include <string>
#include "Exception.h"
#include "BaseVirtualTransport.h"
#include "Logger.h"
#include "ThreadUtil.h"

namespace coconut { 

class COCONUT_API BufferedTransport : public BaseVirtualTransport {
public:
	BufferedTransport() : readPos_(0) { 
		LOG_TRACE("BufferedTransport : %p", this);
	}

	~BufferedTransport() {
		LOG_TRACE("~BufferedTransport : %p", this);
	}

public:
	const char *className() {
		return "BufferedTransport";
	}
	
	int write(const void* ptr, size_t size) {
		ScopedMutexLock(lock_);
		buffer_.append((const char*)ptr, size);
		return size;
	}

	int read(std::string &data, size_t size) {
		ScopedMutexLock(lock_);
		if(remainingSize() < size) {
			throw ProtocolException("read size exceed rest buffer size");
		}
		data.assign((const char *)currentPtr(), size);
		fastforward(size);
		return size;
	}

	int read(void *ptr, size_t size) {
		ScopedMutexLock(lock_);
		if(remainingSize() < size) {
			throw ProtocolException("read size exceed rest buffer size");
		}
		memcpy(ptr, currentPtr(), size);
		fastforward(size);
		return size;
	}

public:
	size_t totalSize() {
		return buffer_.size();
	}

	size_t remainingSize() {
		return buffer_.size() - readPos_;
	}

	const void * currentPtr() {
		return buffer_.c_str() + readPos_;
	}

	size_t readPos() {
		return readPos_;
	}

	void clear() {
		ScopedMutexLock(lock_);
		readPos_ = 0;
		buffer_.clear();
	}

	void rewind(size_t size) {
		ScopedMutexLock(lock_);
		int temp = readPos_ - size;
		if(temp < 0) {
			throw ProtocolException("readpos less than 0 by rewind");
		}
		readPos_ = temp;
	}

	void fastforward(size_t size) {
		ScopedMutexLock(lock_);
		int temp = readPos_ + size;
		if(temp > (int)buffer_.size())
			throw ProtocolException("readpos exceed buffer size by fastforward");
		readPos_ = temp;
	}

private:
	std::string buffer_;
	int readPos_;
	/*int writePos_;*/	// write position is not necessary by using std::string
	Mutex lock_;
};

}

