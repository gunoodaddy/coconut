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

using namespace coconut;
using namespace coconut::protocol;

class TestHttpController : public coconut::HttpClient::EventHandler
{
	virtual void onHttpClient_ReceivedChunked(HttpClient *client, int receivedsize) { 
		LOG_INFO("TestHttpController received size : %d byte\n", receivedsize);
	}

	virtual void onHttpClient_Error(HttpClient *client, coconut::HttpClient::ErrorCode errorcode) {
		LOG_INFO("TestHttpController onError : %d\n", errorcode);
	}

	virtual void onHttpClient_Response(HttpClient *client, int rescode) {
		LOG_INFO("TestHttpController onResponse [this = %p], rescode = %d, size = %d\n", 
				this, rescode, client->responseBodySize());

		printf("%s\n", (char *)client->responseBody());

		// program gracefully exit!
		client->ioService()->ioServiceContainer()->stop();	
	}
};

int main(int argc, char **argv) {
	if(argc < 2) {
		printf("usage : %s [port]\n", argv[0]);
		return -1;
	}

	IOServiceContainer ioServiceContainer;
	ioServiceContainer.initialize();

	try {
		char uri[1024] = {0, };
		sprintf(uri, "http://127.0.0.1:%d/", atoi(argv[1]));

		TestHttpController controller;
		HttpClient client(ioServiceContainer.ioServiceByRoundRobin());
		HttpParameter param;
		param.addParameter("id", "gunoodaddy");
		param.addParameter("age", "32");
		param.addParameter("gender", "Man");
		param.addParameter("item[]", "car");
		param.addParameter("item[]", "ipad3");
		client.setEventHandler(&controller);
		client.request(coconut::HTTP_POST, uri, &param, 20);
		//client.request(coconut::HTTP_GET, uri, &param, 20);

		ioServiceContainer.run();
		LOG_INFO("bye~");
		return 0;
	} catch(Exception &e) {
		LOG_FATAL("Exception emitted : %s\n", e.what());
	}
	return 0;
}


