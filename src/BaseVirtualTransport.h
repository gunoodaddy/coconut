#pragma once

#include "config.h"

namespace coconut {

class COCONUT_API BaseVirtualTransport {
public:
	virtual ~BaseVirtualTransport() { }

	virtual const char *className() = 0;
	virtual int read(std::string &data, size_t size) = 0;
	virtual int read(void *data, size_t size)  = 0;
	virtual int write(const void *data, size_t size) = 0;
};

}
