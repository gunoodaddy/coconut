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

#include "BaseAddress.h"
#include <string>
#if defined(WIN32)
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#include <sys/un.h>
#endif

namespace coconut {

class IPv4Address : public BaseAddress {
public:
	IPv4Address() : port_(0) { }

	IPv4Address(struct sockaddr_in *sin) { 
		setSocketAddress(sin);
	}

#if ! defined(WIN32)
	IPv4Address(struct sockaddr_un *sun) { 
		setSocketAddress(sun);
	}

	void setSocketAddress(struct sockaddr_un *sun) {
		ip_ = sun->sun_path;
		port_ = 0;
	}
#endif

	void setSocketAddress(struct sockaddr_in *sin) {
		ip_ = inet_ntoa(sin->sin_addr);
		port_ = ntohs(sin->sin_port);
	}

	void setSocketAddress(const char* ip, int port) {
		ip_ = ip;
		port_ = port;
	}

	const char *ip() const {
		return ip_.c_str();
	}

	int port() const {
		return port_;
	}

private:
	std::string ip_;	
	int port_;
};

}

