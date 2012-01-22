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
#include "NetworkHelper.h"
#include "IOServiceContainer.h"
#include "LineController.h"

#define BIND_PORT	8765

int g_maxSendCount = 0;

class MyClientController : public coconut::LineController {
public:
	MyClientController(int id, int sendCount) : id_(id), sendCount_(sendCount), currSentCount_(0), recvLineCount_(0) {
	}

	void sendMessage() {
		if(++currSentCount_ <= sendCount_) {
			writeLine("HELLO");
		}
	}
	virtual void onLineReceived(const char *line) {
		recvLineCount_++;
		LOG_DEBUG("LINE RECEIVED : [%s] TOTAL : %d\n", line, recvLineCount_);

		sendMessage();

		if(recvLineCount_ == sendCount_) {
			struct timeval tvEnd;
			gettimeofday(&tvEnd, NULL);

			double diffMsec = (double)tvEnd.tv_sec - tvStart_.tv_sec + ((tvEnd.tv_usec - tvStart_.tv_usec) / 1000000.);
			LOG_INFO("[%d:%p] Test OK! %f msec\n", id_, (void *)pthread_self(), diffMsec);
		}
	}

	virtual void onError(int error, const char *strerror) {
		LOG_INFO("onError emitted..\n");
		setTimer(1, 2000, false);
	}

	virtual void onClosed() { 
		LOG_INFO("onClose emitted..\n");
		setTimer(1, 2000, false);
		socket()->write((const void *)"PENDING1\n", 9);
		socket()->write((const void *)"PENDING2\n", 9);
	}

	virtual void onConnected() {
		LOG_INFO("onConnected emitted..\n");

		gettimeofday(&tvStart_, NULL);
		sendMessage();
	}

	virtual void onTimer(unsigned short id) {
		LOG_DEBUG("onTimer emitted.. %d\n", id);

		// reconnect!
		if(id == 1)
			tcpSocket()->connect();
	}
	
private:
	int id_;
	int sendCount_;
	int currSentCount_;
	int recvLineCount_;
	struct timeval tvStart_;
};


int main(int argc, char **argv) {

	if(argc < 4) {
		printf("usage : %s [port] [client-count] [send-count]\n", argv[0]);
		return -1;
	}
	int port = atoi(argv[1]);
	int clientCount = atoi(argv[2]);
	g_maxSendCount = atoi(argv[3]);

	coconut::IOServiceContainer ioServiceContainer(clientCount);
	ioServiceContainer.initialize();

	try {

		std::vector<boost::shared_ptr<coconut::ClientController> > clients;
		for(int i = 0; i < clientCount; i++) {
			boost::shared_ptr<MyClientController> controller(new MyClientController(i, g_maxSendCount));
			coconut::NetworkHelper::connectTcp(&ioServiceContainer, "localhost", port, controller);

			clients.push_back(controller);
		}

		// event loop start!
		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		LOG_DEBUG("Exception emitted : %s\n", e.what());
	}

	// exit..
}

