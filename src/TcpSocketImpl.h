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

//#include "BaseObjectAllocator.h"

namespace coconut {

class TcpSocketImpl {
public:
	virtual ~TcpSocketImpl() { }

public:
	virtual coconut_socket_t socketFD() = 0;

	virtual void initialize(TcpSocket *owner) = 0;
	virtual void connect() = 0;
	virtual void connect(const char *host, int port, int timeout = 0) = 0;
	virtual void connectUnix(const char *path, int timeout = 0) = 0;

	virtual void attachSocketHandle(coconut_socket_t fd, bool doInstallFlag = true) = 0;

	virtual int read(void *data, size_t size) = 0;
	virtual int read(std::string &data, size_t size) = 0;

	virtual int write(const void *data, size_t size) = 0;

	virtual void close() = 0;
	virtual void closeAfterAllSent() = 0;

	virtual void checkResponseSocket(int res) = 0;

	virtual void install() = 0;

	virtual const BaseAddress * peerAddress() = 0;
	virtual const BaseAddress * sockAddress() = 0;
};

}
