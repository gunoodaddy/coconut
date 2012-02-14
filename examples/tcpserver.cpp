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

//#define USE_LINE_CONTROLLER
#ifdef USE_LINE_CONTROLLER
class MyClientController : public coconut::LineController {
	virtual void onLineReceived(const char *line) {
		writeLine(line);
	}
};
#else
class MyClientController : public coconut::BinaryController {
	virtual void onReceivedData(const void *data, int size) {
		socket()->write(data, size);	
	}
};
#endif

class MyServerController : public coconut::ServerController {
	virtual boost::shared_ptr<coconut::ClientController> onAccept(boost::shared_ptr<coconut::TcpSocket> socket) {
		boost::shared_ptr<MyClientController> newController(new MyClientController); 
		return newController;
	}
};

int main(int argc, char **argv) {
	if(argc < 3) {
		printf("usage : %s [port] [thread-count] [verbose:1,0]\n", argv[0]);
		return -1;
	}
	int port = atoi(argv[1]);
	int threadCount = atoi(argv[2]);
	if(argc > 3 && atoi(argv[3]) == 1) {
		coconut::logger::setLogLevel(coconut::logger::LEVEL_TRACE);
		coconut::setEnableDebugMode();
	}

	coconut::IOServiceContainer ioServiceContainer(threadCount);
	ioServiceContainer.initialize();
	try {
		boost::shared_ptr<MyServerController> serverController(new MyServerController);

		coconut::NetworkHelper::listenTcp(&ioServiceContainer, port, serverController);

		LOG_INFO("tcpserver started..");
		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		LOG_FATAL("Exception emitted : %s\n", e.what());
	}
	return 0;
}
