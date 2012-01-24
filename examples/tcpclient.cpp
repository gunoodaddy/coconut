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
#include "Logger.h"
#include "ThreadUtil.h"

#define BIND_PORT	8765

size_t MAX_CLIENT_COUNT = 0;
size_t MAX_SEND_COUNT = 0;
int TOTAL_RECV_COUNT = 0;
double gNextProg = 0;
coconut::Mutex gLock;

std::vector<double> g_resultList;
boost::uint32_t gRecvedCount = 0;

static void showResult() {
	double avgSec = 0.0f;
	double tps = 0.0f;
	for(size_t i = 0; i < g_resultList.size(); i++) {
		avgSec += g_resultList[i];	
	}
	avgSec /= MAX_CLIENT_COUNT;

	tps = TOTAL_RECV_COUNT / (avgSec);

	printf("\n");
	LOG_INFO("All clients completed : avg = %.2f sec, server tps = %.2f\n", avgSec, tps);
}

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
		coconut::atomicIncreaseInt32(&gRecvedCount);

		LOG_DEBUG("LINE RECEIVED : [%s] TOTAL : %d\n", line, recvLineCount_);

		double curProg = gRecvedCount * 100 / (double)TOTAL_RECV_COUNT;
		if(curProg >= gNextProg) {
			struct timeval tvEnd;
			gettimeofday(&tvEnd, NULL);
			double diffSec = (double)tvEnd.tv_sec - tvStart_.tv_sec + ((tvEnd.tv_usec - tvStart_.tv_usec) / 1000000.);
			int tps = (int)(gRecvedCount / diffSec);

			printf("\r%.2f%%, %d tps", curProg, tps);
			fflush(stdout);
			gNextProg += 0.1f;
			if(gNextProg > 100)
				gNextProg = 100.0f;
		}

		sendMessage();

		if(recvLineCount_ == sendCount_) {
			struct timeval tvEnd;
			gettimeofday(&tvEnd, NULL);

			double diffSec = (double)tvEnd.tv_sec - tvStart_.tv_sec + ((tvEnd.tv_usec - tvStart_.tv_usec) / 1000000.);
			LOG_DEBUG("[%d:%p] Test OK! %f msec\n", id_, (void *)pthread_self(), diffSec);

			gLock.lock();
			g_resultList.push_back(diffSec);
			gLock.unlock();

			if(g_resultList.size() == MAX_CLIENT_COUNT) {
				showResult();
				ioServiceContainer()->stop();
			}
		}
	}

	virtual void onError(int error, const char *strerror) {
		LOG_ERROR("onError emitted..\n");
		setTimer(1, 2000, false);
	}

	virtual void onClosed() { 
		LOG_ERROR("onClose emitted..\n");
		setTimer(1, 2000, false);
		socket()->write((const void *)"PENDING1\n", 9);
		socket()->write((const void *)"PENDING2\n", 9);
	}

	virtual void onConnected() {
		LOG_DEBUG("onConnected emitted..\n");

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


void coconutLog(coconut::logger::LogLevel level, const char *fileName, int fileLine, const char *functionName, const char *logmsg, bool internalLog) {
	printf("[COCONUT] <%d> %s\n", level, logmsg);
}


int main(int argc, char **argv) {

	if(argc < 4) {
		printf("usage : %s [address] [port] [client-count] [send-count] [log-level]\n", argv[0]);
		return -1;
	}
	std::string address = "localhost";
	int port = 8000;
	coconut::logger::LogLevel logLevel = coconut::logger::LEVEL_INFO;
	if(argc > 1)
		address = argv[1];
	if(argc > 2)
		port = atoi(argv[2]);
	if(argc > 3)
		MAX_CLIENT_COUNT = atoi(argv[3]);
	if(argc > 4)
		MAX_SEND_COUNT = atoi(argv[4]);
	if(argc > 5)
		logLevel = (coconut::logger::LogLevel)atoi(argv[5]);

	TOTAL_RECV_COUNT = MAX_CLIENT_COUNT * MAX_SEND_COUNT;

	// logger setting
	{
		coconut::logger::LogHookCallback logCallback;
		logCallback.trace = coconutLog;
		logCallback.debug = coconutLog;
		logCallback.info = coconutLog;
		logCallback.warning = coconutLog;
		logCallback.error = coconutLog;
		logCallback.fatal = coconutLog;
		coconut::logger::setLogHookFunctionCallback(logCallback);
		coconut::logger::setLogLevel(logLevel);
	}

	coconut::IOServiceContainer ioServiceContainer(MAX_CLIENT_COUNT);
	ioServiceContainer.initialize();

	try {

		std::vector<boost::shared_ptr<coconut::ClientController> > clients;
		for(size_t i = 0; i < MAX_CLIENT_COUNT; i++) {
			boost::shared_ptr<MyClientController> controller(new MyClientController(i, MAX_SEND_COUNT));
			coconut::NetworkHelper::connectTcp(&ioServiceContainer, address.c_str(), port, controller);

			clients.push_back(controller);
		}

		// event loop start!
		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		LOG_DEBUG("Exception emitted : %s\n", e.what());
	}

	// exit..
}

