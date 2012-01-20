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

