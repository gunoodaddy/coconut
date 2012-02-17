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

class MyClientController : public coconut::BinaryController {
	virtual void onReceivedData(const void *data, int size) {
		socket()->write(data, size);	
	}
};


class MyTcpClientAcceptor : public coconut::FileDescriptorController {
public:
	typedef std::set<boost::shared_ptr<BaseController> > SetClients_t;

	virtual void onConnected() {
		LOG_INFO("MyUnixServerController::onConnected called");
	}
	virtual void onControllerEvent_ClosedConnection( boost::shared_ptr<BaseController> controller, int error) {
		LOG_ERROR("MyUnixServerController client closed event emitted.. error = %d", error);

		SetClients_t::iterator it = clients_.find(controller);
		if(clients_.end() != it) {
			clients_.erase(it);
			LOG_INFO("MyUnixServerController client find! erase it");
		}
	}

	virtual void onDescriptorReceived(int fd) {
		LOG_DEBUG("MyTcpClientAcceptor::onDescriptorReceived called : fd = %d", fd);

		boost::shared_ptr<MyClientController> client = boost::shared_ptr<MyClientController>(new MyClientController);
		coconut::NetworkHelper::attachTcp(ioService(), fd, client);
		client->eventClosedConnection()->registerObserver(this);

		clients_.insert(client);
	}

private:
	SetClients_t clients_;
};


int main(int argc, char **argv) {
	if(argc < 3) {
		printf("usage : %s [thread-count] [verbose:1,0]\n", argv[0]);
		return -1;
	}
	int threadCount = atoi(argv[1]);
	if(threadCount <= 0) threadCount = 1;
	if(argc > 2 && atoi(argv[2]) == 1) {
		coconut::logger::setLogLevel(coconut::logger::LEVEL_TRACE);
		coconut::setEnableDebugMode();
	}

	coconut::IOServiceContainer ioServiceContainer(threadCount);
	ioServiceContainer.initialize();
	try {
		std::vector<boost::shared_ptr<MyTcpClientAcceptor> > list;
		for(int i = 0; i < threadCount; i++) {
			boost::shared_ptr<MyTcpClientAcceptor> client = boost::shared_ptr<MyTcpClientAcceptor>(new MyTcpClientAcceptor);
			coconut::NetworkHelper::connectUnix(&ioServiceContainer, "fdserver.sock", client);
			list.push_back(client);
		}

		LOG_INFO("tcpserver started..");
		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		LOG_FATAL("Exception emitted : %s\n", e.what());
	}
	return 0;
}
