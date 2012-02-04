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
#include <fcntl.h>

class MyFDController : public coconut::FileDescriptorController {

public:
	MyFDController() : fd_(0) { 
	}
	
	virtual void onInitialized() {
		printf("MyFDController::onInitialized emitted.. %d\n", fd_);

		if(fd_ > 0)
			writeDescriptor(fd_);
	}
	
	virtual void onClosed() { 
		printf("MyFDController::onClosed()\n");
	}

	virtual void onDescriptorReceived(int fd) {
		/*
		printf("MyFDController::onDescriptorReceived emitted.. %d\n", fd);

		if(fd <= 0)
			return;

		while(true) {
			char buff[1024] = {0, };
			if(read(fd, buff, sizeof(buff)) > 0) {
				printf("%s", buff);
				continue;
			}
			break;
		}
		::close(fd);
		*/
	}

	void setTestFd(int fd) {
		fd_ = fd;
	}

private:
	int fd_;
};


class MyUnixServerController : public coconut::ServerController {
public:
	virtual void onInitialized() {

		fd_ = open("test.txt", O_RDONLY);
		printf("MyUnixServerController onInitialized file descriptor = %d\n", fd_);
	}

	virtual boost::shared_ptr<coconut::ClientController> onAccept(boost::shared_ptr<coconut::TcpSocket> socket) {

		boost::shared_ptr<MyFDController> newController(new MyFDController); 
		newController->setTestFd(fd_);
		return newController;
	}

private:
	int fd_;
};


int main(void) {
	coconut::IOServiceContainer ioServiceContainer;
	ioServiceContainer.initialize();

	try {
		// unix server test
		boost::shared_ptr<MyUnixServerController> unixServerController(new MyUnixServerController);
		coconut::NetworkHelper::listenUnix(&ioServiceContainer, "fdserver.sock", unixServerController);

		// event loop start!
		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		printf("Exception emitted : %s\n", e.what());
	}

	// exit..
}

