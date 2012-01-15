#include "Coconut.h"
#include <fcntl.h>
#include "NetworkHelper.h"
#include "IOServiceContainer.h"
#include "FileDescriptorController.h"

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

