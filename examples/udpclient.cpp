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
#include <arpa/inet.h>
#include "NetworkHelper.h"
#include "IOServiceContainer.h"

#define UDP_PORT	7777

class MyUdpClientController : public coconut::BinaryController {
public:
	void onReceivedDatagram(const void *data, int size, const struct sockaddr_in *sin) {
		printf("onReadFrom emitted.. %s:%d from %s:%d\n", (char*)data, size, inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));
	}
};


int main(void) {
	coconut::IOServiceContainer ioServiceContainer;

	boost::shared_ptr<MyUdpClientController> udpclient(new MyUdpClientController);
	coconut::NetworkHelper::bindUdp(&ioServiceContainer, 0/*for client*/, udpclient);
	udpclient->udpSocket()->writeTo("HELLO", 5, "localhost", UDP_PORT);

	ioServiceContainer.run();
}

