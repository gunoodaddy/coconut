#pragma once

#include <string>
#include "BaseVirtualTransport.h"
#include "ThreadUtil.h"

namespace coconut { 

class COCONUT_API BufferedTransport : public BaseVirtualTransport {
public:
	BufferedTransport();
	~BufferedTransport();

public:
	const char *className() {
		return "BufferedTransport";
	}
	
	int write(const void* ptr, size_t size);
	int read(std::string &data, size_t size);
	int read(void *ptr, size_t size);
	const void * peek(size_t &size);
	void ackReadSize(size_t size);

public:
	size_t totalSize();
	size_t remainingSize();
	size_t readPos();

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

