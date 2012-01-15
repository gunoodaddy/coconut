#include "Coconut.h"
#include "NetworkHelper.h"
#include "IOServiceContainer.h"
#include "FileDescriptorController.h"

class MyFDController : public coconut::FileDescriptorController {
	virtual void onDescriptorReceived(int fd) {
		printf("MyFDController onDescriptorReceived emitted.. %d\n", fd);

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
	}
};


int main(void) {
	coconut::IOServiceContainer ioServiceContainer;
	ioServiceContainer.initialize();

	try {
		// unix client test
		boost::shared_ptr<MyFDController> unixClientController(new MyFDController);
		coconut::NetworkHelper::connectUnix(&ioServiceContainer, "fdserver.sock", unixClientController);

		// event loop start!
		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		printf("Exception emitted : %s\n", e.what());
	}

	// exit..
}

