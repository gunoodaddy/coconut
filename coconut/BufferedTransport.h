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

