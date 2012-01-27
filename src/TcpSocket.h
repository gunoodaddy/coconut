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

#include "BaseSocket.h"

struct kbuffer;

namespace coconut {

class IOService;
class TcpSocketImpl;

class COCONUT_API TcpSocket : public BaseSocket {
public:
	TcpSocket(boost::shared_ptr<IOService> ioService);
	~TcpSocket();

public:
	int socketFD();
	void connect();
	void connect(const char *host, int port, int timeout = 0);
	void connectUnix(const char *path, int timeout = 0);
	void attachSocketHandle(coconut_socket_t fd, bool doInstallFlag = true);
	int read(void *data, size_t size);
	int read(std::string &data, size_t size);
	int write(const void *data, size_t size);
	int writeString(const std::string &data);
	void close();
	void checkResponseSocket(int res);
	void install();

	virtual const BaseAddress * peerAddress();
	virtual const BaseAddress * sockAddress();

private:
	TcpSocketImpl *impl_;
};

}
