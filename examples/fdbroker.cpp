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

typedef std::vector<boost::shared_ptr<coconut::FileDescriptorController> > VectorUnixClient_t;
VectorUnixClient_t gUnixClients;

class MyUnixServerController : public coconut::ServerController {
	public:
		virtual void onControllerEvent_ClosedConnection(
				boost::shared_ptr<BaseController> controller,
				int error) {
			LOG_ERROR("MyUnixServerController client closed event emitted.. error = %d", error);
			VectorUnixClient_t::iterator it = std::find(gUnixClients.begin(), gUnixClients.end(), controller);
			if(it != gUnixClients.end()) {
				LOG_ERROR("got it! remove that client controller!");
				gUnixClients.erase(it);
			}
		}

		virtual boost::shared_ptr<coconut::ClientController> onAccept(boost::shared_ptr<coconut::TcpSocket> socket) {
			boost::shared_ptr<coconut::FileDescriptorController> newController(new coconut::FileDescriptorController); 
			gUnixClients.push_back(newController);
			LOG_ERROR("MyUnixServerController::onAccept emitted.. : count = %d", gUnixClients.size());
		return newController;
	}
};

class MyServerController : public coconut::ServerController {
public:
	MyServerController() : currIndex_(0) { }
	virtual bool onHookAccept(coconut_socket_t newSocket) {
		LOG_DEBUG("MyServerController::onHookAccept() called : newsocket = %d\n", newSocket);
		if(gUnixClients.size() <= 0)
			return true;

		if(currIndex_ >= gUnixClients.size()) currIndex_ = 0;

		boost::shared_ptr<coconut::FileDescriptorController> ctrl = gUnixClients[currIndex_];
		ctrl->writeDescriptor(newSocket);

		currIndex_++;
		return true;
	}

private:
	size_t currIndex_;	// for round robin
};


int main(int argc, char **argv) {
	if(argc < 2) {
		printf("usage : %s [port] [verbose:1,0]\n", argv[0]);
		return -1;
	}

	int port = atoi(argv[1]);
	if(argc > 2 && atoi(argv[2]) == 1) {
		coconut::logger::setLogLevel(coconut::logger::LEVEL_TRACE);
		coconut::setEnableDebugMode();
	}

	coconut::IOServiceContainer ioServiceContainer;
	ioServiceContainer.initialize();

	try {
		// unix server
		boost::shared_ptr<MyUnixServerController> unixServerController(new MyUnixServerController);
		coconut::NetworkHelper::listenUnix(&ioServiceContainer, "fdserver.sock", unixServerController);

		// tcp server
		boost::shared_ptr<MyServerController> serverController(new MyServerController);
		coconut::NetworkHelper::listenTcp(&ioServiceContainer, port, serverController);

		// event loop start!
		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		printf("Exception emitted : %s\n", e.what());
	}

	// exit..
}

