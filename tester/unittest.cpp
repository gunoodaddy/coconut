#include "Coconut.h"
#include <assert.h>
#if defined(WIN32)
#include <conio.h>
#endif
#include <fcntl.h>
#include "NetworkHelper.h"
#include "IOServiceContainer.h"
#include "LineController.h"
#include "FrameController.h"
#include "FileDescriptorController.h"
#include "Logger.h"
#include "log4cxxutil.h"

#if defined(USE_LOG4CXX)
#include <log4cxx/logger.h> 
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
using namespace log4cxx;
LoggerPtr gLogger(Logger::getLogger("MyApp"));
#endif

using namespace coconut;
using namespace coconut::protocol;

static const char *REDIS_ADDRESS = "61.247.198.102";
static int gPortBase = 8000;

namespace TestUDPAndLineProtocol {
	static const int UDP_PORT = 1234;
	static int retryCnt = 3;

	class TestUdpClientController : public coconut::LineController {
	public:
		virtual void onInitialized() {
			setTimer(1, 1000, true);
			sendTest();
		}

		virtual void onLineReceived(const char *line) {
			MY_LOG4CXX_INFO(gLogger, "TestUdpClientController LINE RECEIVED : [%s] port : %d\n", 
					line, ntohs(udpSocket()->lastClientAddress()->sin_port));

			MY_LOG4CXX_INFO(gLogger, "************ Test Success ************");
			ioServiceContainer()->stop();	// test success
		}

		void sendTest() {
			udpSocket()->writeTo("UDP HELLO\r\n", 11, "localhost", UDP_PORT);
		}

		virtual void onTimer(unsigned short id) {
			// retry
			assert(retryCnt-- > 0 && "time out. failed to receive udp packet"); 
			sendTest();
		}
	};

	class TestUdpServerController : public coconut::LineController {
	public:
		virtual void onLineReceived(const char *line) {
			MY_LOG4CXX_INFO(gLogger, "TestUdpServerController LINE RECEIVED : [%s]\n", line);
			writeLine(line);
		}
	};

	bool doTest() {
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");
		MY_LOG4CXX_INFO(gLogger, "UDP And Line Protocol Test");
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");

		boost::shared_ptr<BaseIOServiceContainer> ioServiceContainer;
		ioServiceContainer = boost::shared_ptr<BaseIOServiceContainer>(new IOServiceContainer);

		ioServiceContainer->initialize();

		try {
			boost::shared_ptr<TestUdpServerController> udpServerController(new TestUdpServerController);
			coconut::NetworkHelper::bindUdp(ioServiceContainer.get(), UDP_PORT, udpServerController);

			boost::shared_ptr<TestUdpClientController> controller(new TestUdpClientController);
			coconut::NetworkHelper::bindUdp(ioServiceContainer.get(), 0, controller);

			ioServiceContainer->run();

			return true;
		} catch(Exception &e) {
			printf("Exception emitted : %s\n", e.what());
		}
		return false;
	}
}


namespace TestHttpRequestGet
{
	class TestHttpController : public coconut::HttpRequestController
	{
		virtual void onReceivedChucked(int receivedsize) { 
			MY_LOG4CXX_INFO(gLogger, "TestHttpController received size : %d byte\n", receivedsize);
		}

		virtual void onError(coconut::HttpRequest::ErrorCode errorcode) {
			MY_LOG4CXX_INFO(gLogger, "TestHttpController onError : %d\n", errorcode);
		}

		virtual void onResponse(int rescode) {
			MY_LOG4CXX_INFO(gLogger, "TestHttpController onResponse [this = %p], rescode = %d, size = %d\n", 
				this, rescode, httpRequest()->responseBodySize());

			printf("%s\n", (char *)httpRequest()->responseBody());

			MY_LOG4CXX_INFO(gLogger, "************ Test Success ************");
			ioServiceContainer()->stop();	// test success
		}
	};

	bool doTest() {
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");
		MY_LOG4CXX_INFO(gLogger, "Http Request GET Test");
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");

		boost::shared_ptr<BaseIOServiceContainer> ioServiceContainer;
		ioServiceContainer = boost::shared_ptr<BaseIOServiceContainer>(new IOServiceContainer);

		ioServiceContainer->initialize();

		try {
			std::string uri;
			uri = "http://119.205.238.162:8081/test.php";
			boost::shared_ptr<TestHttpController> controller(new TestHttpController);
			coconut::NetworkHelper::httpRequest(ioServiceContainer.get(), coconut::HTTP_POST, uri.c_str(), 20, NULL, controller);

			ioServiceContainer->run();
			return true;
		} catch(Exception &e) {
			printf("Exception emitted : %s\n", e.what());
		}
		return false;
	}

}

namespace TestLineProtocol {

	class TestClientController : public LineController {
		virtual void onConnected() {
			MY_LOG4CXX_DEBUG(gLogger, "onConnected called : %d", socket()->socketFD());
			recvedLine_ = 0;
			socket()->write("HELLO\r\n", 7);
			socket()->write("HELLO\r\n", 7);
		}

		virtual void onLineReceived(const char *line) {
			MY_LOG4CXX_DEBUG(gLogger, "[CLIENT] onLineReceived called : %s", line);

			if(strcmp(line, "How's it going?") == 0) {
				recvedLine_++;
				if(recvedLine_ == 2) {
					MY_LOG4CXX_INFO(gLogger, "************ Test Success ************");
					ioServiceContainer()->stop();	// test success
				}
			}
		}

		private:
		int recvedLine_;
	};

	class TestServerClientController : public LineController {
		virtual void onInitialized() {
			MY_LOG4CXX_DEBUG(gLogger, "onInitialized called : %d", socket()->socketFD());
			//socket()->write("HELLO\r\n", 7);
		}
		virtual void onLineReceived(const char *line) {
			MY_LOG4CXX_DEBUG(gLogger, "[SERVER] onLineReceived called : %s", line);
			writeLine("How's it going?");
		}
	};

	class TestServerController : public ServerController {

		virtual boost::shared_ptr<ClientController> onAccept(boost::shared_ptr<TcpSocket> socket) {
			
			boost::shared_ptr<TestServerClientController> newController(new TestServerClientController); 
			MY_LOG4CXX_DEBUG(gLogger, "[SERVER] onAccept called : %p", newController.get());
			return newController;
		}
	};

	bool doTest() {
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");
		MY_LOG4CXX_INFO(gLogger, "Line Protocol Test");
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");

		boost::shared_ptr<IOServiceContainer> ioServiceContainer;
		ioServiceContainer = boost::shared_ptr<IOServiceContainer>(new IOServiceContainer);
		//	ioServiceContainer = boost::shared_ptr<BaseIOServiceContainer>(new IOServiceContainer(threadCount));

#if defined(WIN32)
		ioServiceContainer->turnOnIOCP(1);
#endif
		ioServiceContainer->initialize();

		try {
			boost::shared_ptr<TestServerController> serverController(new TestServerController);
			NetworkHelper::listenTcp(ioServiceContainer.get(), gPortBase, serverController);

			boost::shared_ptr<TestClientController> clientController(new TestClientController);
			NetworkHelper::connectTcp(ioServiceContainer.get(), "localhost", gPortBase, clientController);

			gPortBase++;

			ioServiceContainer->run();
			return true;
		} catch(Exception &e) {
			printf("Exception emitted : %s\n", e.what());
		}
		return false;
	}
}


namespace TestFrameProtocol {
	static const int COMMAND = 813;

	class TestClientController : public FrameController {

		virtual void onConnected() {
			MY_LOG4CXX_DEBUG(gLogger, "onConnected called");
			recvedLine_ = 0;

			FrameHeader header(COMMAND, 0);
			writeFrame(header, "0PAYLOAD", 8);	
			writeFrame(header, "1PAYLOAD", 8);	
			writeFrame(header, "2PAYLOAD", 8);	
			writeFrame(header, "3PAYLOAD", 8);	
			writeFrame(header, "4PAYLOAD", 8);	
		}

		virtual void onFrameReceived(boost::shared_ptr<FrameProtocol> prot) {
			MY_LOG4CXX_DEBUG(gLogger, "[CLIENT] onFrameReceived called : %d %s:%d", prot->header().command(), prot->payloadPtr(), prot->payloadSize());
			
			std::string payload;
			payload.assign((char *)prot->payloadPtr(), prot->payloadSize());

			char expactedPayload[1024] = {0, };
			sprintf(expactedPayload, "RESPOSE : %dPAYLOAD", recvedLine_);

			assert(COMMAND + 1 == prot->header().command());
			assert(strcmp(expactedPayload, payload.c_str()) == 0);

			recvedLine_++;
			if(recvedLine_ == 5) {
				MY_LOG4CXX_INFO(gLogger, "************ Test Success ************");
				ioServiceContainer()->stop();	// test success
			}
		}

		private:
		int recvedLine_;
	};

	class TestServerClientController : public FrameController {
		virtual void onFrameReceived(boost::shared_ptr<FrameProtocol> prot) {
			MY_LOG4CXX_DEBUG(gLogger, "[SERVER] onFrameReceived called : %d %s:%d", prot->header().command(), prot->payloadPtr(), prot->payloadSize());
			
			std::string payload;
			payload.assign((char *)prot->payloadPtr(), prot->payloadSize());
			char resPayload[1024] = {0, };
			sprintf(resPayload, "RESPOSE : %s", payload.c_str());

			FrameHeader header(prot->header().command() + 1, 1);
			writeFrame(header, resPayload, strlen(resPayload));
		}
	};

	class TestServerController : public ServerController {
		virtual boost::shared_ptr<ClientController> onAccept(boost::shared_ptr<TcpSocket> socket) {
			boost::shared_ptr<TestServerClientController> newController(new TestServerClientController); 
			return newController;
		}
	};

	bool doTest() {
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");
		MY_LOG4CXX_INFO(gLogger, "Frame Protocol Test");
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");

		boost::shared_ptr<BaseIOServiceContainer> ioServiceContainer;
		ioServiceContainer = boost::shared_ptr<BaseIOServiceContainer>(new IOServiceContainer);
		ioServiceContainer->initialize();

		try {
			boost::shared_ptr<TestServerController> serverController(new TestServerController);
			NetworkHelper::listenTcp(ioServiceContainer.get(), gPortBase, serverController);

			boost::shared_ptr<TestClientController> clientController(new TestClientController);
			NetworkHelper::connectTcp(ioServiceContainer.get(), "localhost", gPortBase++, clientController);

			ioServiceContainer->run();
			return true;
		} catch(Exception &e) {
			printf("Exception emitted : %s\n", e.what());
		}
		return false;
	}
}


namespace TestFrameAndStringListProtocol {
	static const int COMMAND = 813;

	class TestClientController : public FrameController {

		virtual void onConnected() {
			MY_LOG4CXX_DEBUG(gLogger, "onConnected called");
			recvedLine_ = 0;

			boost::shared_ptr<StringListProtocol> slprot(new StringListProtocol);
			slprot->addString("ID1");
			slprot->addString("ID2");
			slprot->addString("ID3");
			slprot->processSerialize();

			FrameHeader header(COMMAND, 0);
			writeFrame(header, slprot->writingBuffer());
			writeFrame(header, slprot->writingBuffer());
		}

		virtual void onFrameReceived(boost::shared_ptr<FrameProtocol> prot) {
			MY_LOG4CXX_DEBUG(gLogger, "[CLIENT] onFrameReceived called : %d:%d", prot->header().command(), prot->payloadSize());
			
			boost::shared_ptr<StringListProtocol> slprot(new StringListProtocol(prot));
			slprot->processReadFromPayloadBuffer();

			assert(slprot->isReadComplete());
			for(size_t i = 0; i < slprot->listSize(); i++) {
				MY_LOG4CXX_DEBUG(gLogger, "[CLIENT] list string [%d] = %s\n", i, slprot->stringOf(i).c_str());
			}

			assert(slprot->listSize() == 4);
			assert(slprot->stringOf(0) == "ID1");
			assert(slprot->stringOf(1) == "ID2");
			assert(slprot->stringOf(2) == "ID3");
			assert(slprot->stringOf(3) == "NEW ID4 FROM SERVER");
			recvedLine_++;
			if(recvedLine_ == 2) {
				MY_LOG4CXX_INFO(gLogger, "************ Test Success ************");
				ioServiceContainer()->stop();	// test success
			}
		}

		private:
		int recvedLine_;
	};

	class TestServerClientController : public FrameController {
		virtual void onFrameReceived(boost::shared_ptr<FrameProtocol> prot) {
			MY_LOG4CXX_DEBUG(gLogger, "[SERVER] onFrameReceived called : %d:%d", prot->header().command(), prot->payloadSize());

			boost::shared_ptr<StringListProtocol> slprot(new StringListProtocol(prot));
			slprot->processReadFromPayloadBuffer();

			assert(slprot->isReadComplete());
			assert(slprot->listSize() == 3);
			assert(slprot->stringOf(0) == "ID1");
			assert(slprot->stringOf(1) == "ID2");
			assert(slprot->stringOf(2) == "ID3");
			for(size_t i = 0; i < slprot->listSize(); i++) {
				MY_LOG4CXX_DEBUG(gLogger, "[SERVER] list string [%d] = %s\n", i, slprot->stringOf(i).c_str());
			}

			slprot->addString("NEW ID4 FROM SERVER");
			slprot->processSerialize();
			FrameHeader header(prot->header().command() + 1, 2);
			writeFrame(header, slprot->writingBuffer());
		}
	};

	class TestServerController : public ServerController {
		virtual boost::shared_ptr<ClientController> onAccept(boost::shared_ptr<TcpSocket> socket) {
			boost::shared_ptr<TestServerClientController> newController(new TestServerClientController); 
			return newController;
		}
	};

	bool doTest() {
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");
		MY_LOG4CXX_INFO(gLogger, "Frame And String List Protocol Test");
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");

		boost::shared_ptr<BaseIOServiceContainer> ioServiceContainer;
		ioServiceContainer = boost::shared_ptr<BaseIOServiceContainer>(new IOServiceContainer);
		ioServiceContainer->initialize();

		try {
			boost::shared_ptr<TestServerController> serverController(new TestServerController);
			NetworkHelper::listenTcp(ioServiceContainer.get(), gPortBase, serverController);

			boost::shared_ptr<TestClientController> clientController(new TestClientController);
			NetworkHelper::connectTcp(ioServiceContainer.get(), "localhost", gPortBase, clientController);

			gPortBase++;

			ioServiceContainer->run();
			return true;
		} catch(Exception &e) {
			printf("Exception emitted : %s\n", e.what());
		}
		return false;
	}
}


namespace TestFrameAndStringListAndLineProtocol {
	static const int COMMAND = 813;

	class TestClientController : public FrameController {

		virtual void onConnected() {
			MY_LOG4CXX_DEBUG(gLogger, "onConnected called : %p", this);
			recvedLine_ = 0;

			boost::shared_ptr<StringListProtocol> slprot(new StringListProtocol);
			slprot->addString("ID1");
			slprot->addString("ID2");
			slprot->addString("ID3");
			slprot->addString("ID4");
			slprot->addString("ID5");
			boost::shared_ptr<LineProtocol> lprot(new LineProtocol(slprot));
			lprot->setLine("TEST LINE");
			lprot->processSerialize();

			FrameHeader header(COMMAND, 1);
			writeFrame(header, lprot);
			writeFrame(header, lprot);
			writeFrame(header, lprot);
			writeFrame(header, lprot);
			writeFrame(header, lprot);
			writeFrame(header, lprot);
		}

		virtual void onFrameReceived(boost::shared_ptr<FrameProtocol> prot) {
			MY_LOG4CXX_DEBUG(gLogger, "[CLIENT] onFrameReceived called : %d:%d", prot->header().command(), prot->payloadSize());
			
			boost::shared_ptr<StringListProtocol> slprot(new StringListProtocol(prot));
			boost::shared_ptr<LineProtocol> lprot(new LineProtocol(slprot));
			lprot->processReadFromPayloadBuffer();

			assert(lprot->isReadComplete());
			for(size_t i = 0; i < slprot->listSize(); i++) {
				MY_LOG4CXX_DEBUG(gLogger, "[CLIENT] list string [%d] = %s\n", i, slprot->stringOf(i).c_str());
			}

			if(recvedLine_ % 2 == 0) {
				assert(prot->header().command() == COMMAND);
				assert(prot->header().transactionId() == 1);
			} else {
				assert(prot->header().command() == COMMAND+813);
				assert(prot->header().transactionId() == 2);
			}
			assert(slprot->listSize() == 6);
			assert(slprot->stringOf(0) == "ID1");
			assert(slprot->stringOf(1) == "ID2");
			assert(slprot->stringOf(2) == "ID3");
			assert(slprot->stringOf(3) == "ID4");
			assert(slprot->stringOf(4) == "ID5");
			assert(slprot->stringOf(5) == "NEW ID6 FROM SERVER");
			assert(strcmp(lprot->linePtr(), "TEST LINE RESPONSE") == 0);

			recvedLine_++;
			if(recvedLine_ == 6) {
				MY_LOG4CXX_INFO(gLogger, "************ Test Success ************");
				ioServiceContainer()->stop();	// test success
			}
		}

		private:
		int recvedLine_;
	};

	class TestServerClientController : public FrameController {
		public:
		TestServerClientController() : writeFrameMode(0) { }
		virtual void onFrameReceived(boost::shared_ptr<FrameProtocol> prot) {
			MY_LOG4CXX_DEBUG(gLogger, "[SERVER] onFrameReceived called : %d:%d", prot->header().command(), prot->payloadSize());

			StringListProtocol slprot(prot.get());
			LineProtocol lprot(&slprot);

			lprot.processReadFromPayloadBuffer();
			
			assert(lprot.isReadComplete());
			for(size_t i = 0; i < slprot.listSize(); i++) {
				MY_LOG4CXX_DEBUG(gLogger, "[SERVER] list string [%d] = %s\n", i, slprot.stringOf(i).c_str());
			}

			assert(slprot.listSize() == 5);
			assert(slprot.stringOf(0) == "ID1");
			assert(slprot.stringOf(1) == "ID2");
			assert(slprot.stringOf(2) == "ID3");
			assert(slprot.stringOf(3) == "ID4");
			assert(slprot.stringOf(4) == "ID5");
			assert(strcmp(lprot.linePtr(), "TEST LINE") == 0);

			slprot.addString("NEW ID6 FROM SERVER");
			lprot.setLine("TEST LINE RESPONSE");
			lprot.processSerialize();

			if(writeFrameMode++ % 2 == 0) {
				MY_LOG4CXX_DEBUG(gLogger, "[SERVER] write mode == 0");
				lprot.processWrite(socket());
			} else {
				MY_LOG4CXX_DEBUG(gLogger, "[SERVER] write mode == 1");
				FrameHeader header(prot->header().command() + 813, 2);
				writeFrame(header, &lprot);
			}
		}

	private:
		int writeFrameMode;
	};

	class TestServerController : public ServerController {
		virtual boost::shared_ptr<ClientController> onAccept(boost::shared_ptr<TcpSocket> socket) {
			boost::shared_ptr<TestServerClientController> newController(new TestServerClientController); 
			return newController;
		}
	};

	bool doTest() {
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");
		MY_LOG4CXX_INFO(gLogger, "Frame And String List And Line Protocol Test");
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");

		boost::shared_ptr<BaseIOServiceContainer> ioServiceContainer;
		ioServiceContainer = boost::shared_ptr<BaseIOServiceContainer>(new IOServiceContainer(2));
		ioServiceContainer->initialize();

		try {
			boost::shared_ptr<TestServerController> serverController(new TestServerController);
			NetworkHelper::listenTcp(ioServiceContainer.get(), gPortBase, serverController);

			boost::shared_ptr<TestClientController> clientController(new TestClientController);
			NetworkHelper::connectTcp(ioServiceContainer.get(), "localhost", gPortBase, clientController);

			gPortBase++;

			ioServiceContainer->run();
			return true;
		} catch(Exception &e) {
			printf("Exception emitted : %s\n", e.what());
		}
		return false;
	}
}

#if ! defined(WIN32)
namespace TestFileDescriptorProtocol {
	class TestFDController : public FileDescriptorController {
		public:
			TestFDController() : fd_(0) { 
			}

			virtual void onInitialized() {
				MY_LOG4CXX_DEBUG(gLogger, "TestFDController onInitialized emitted.. %d -> %d\n", socket()->socketFD(), fd_);

				if(fd_ > 0)
					writeDescriptor(fd_);
			}
			virtual void onDescriptorReceived(int fd) {
				MY_LOG4CXX_DEBUG(gLogger, "TestFDController onDescriptorReceived emitted.. %d\n", fd);

				if(fd <= 0)
					return;

				char buff[1024] = {0, };
				while(true) {
					if(read(fd, buff, sizeof(buff)) > 0) {
						MY_LOG4CXX_DEBUG(gLogger, "%s", buff);
						continue;
					}
					break;
				}

				assert(strstr(buff, "TEST READ FILE"));
				assert(strstr(buff, "// END"));

				close(fd);
				MY_LOG4CXX_INFO(gLogger, "************ Test Success ************");
				ioServiceContainer()->stop();	// test success
			}

			void setTestFd(int fd) {
				fd_ = fd;
			}

		private:
			int fd_;
	};


	class TestUnixServerController : public ServerController {
		public:
			virtual void onInitialized() {
				fd_ = open("test.txt", O_RDONLY);
				MY_LOG4CXX_DEBUG(gLogger, "TestUnixServerController onInitialized file descriptor = %d\n", fd_);
			}

			virtual boost::shared_ptr<ClientController> onAccept(boost::shared_ptr<TcpSocket> socket) {
				boost::shared_ptr<TestFDController> newController(new TestFDController); 
				newController->setTestFd(fd_);
				return newController;
			}

		private:
			int fd_;
	};

	bool doTest() {
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");
		MY_LOG4CXX_INFO(gLogger, "FileDescriptorProtocol Test");
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");

		boost::shared_ptr<BaseIOServiceContainer> ioServiceContainer;
		ioServiceContainer = boost::shared_ptr<BaseIOServiceContainer>(new IOServiceContainer(2));
		ioServiceContainer->initialize();

		try {
			// unix server test
			boost::shared_ptr<TestUnixServerController> unixServerController(new TestUnixServerController);
			NetworkHelper::listenUnix(ioServiceContainer.get(), "server.sock", unixServerController);

			// unix client test
			//		boost::shared_ptr<TestClientController> unixClientController(new TestClientController);
			boost::shared_ptr<TestFDController> unixClientController(new TestFDController);
			NetworkHelper::connectUnix(ioServiceContainer.get(), "server.sock", unixClientController);

			ioServiceContainer->run();
			return true;
		} catch(Exception &e) {
			printf("Exception emitted : %s\n", e.what());
		}
		return false;
	}
}
#endif


namespace TestRedisRequest {
	static const int GET_COUNT = 100;
	//static const int GET_COUNT = 10;

	boost::shared_ptr<RedisController> gRedisCtrl_;

	class TestRedisController : public RedisController {
		public:
			virtual void onConnected() {
				MY_LOG4CXX_INFO(gLogger,"!! REDIS CONNECTED");
			}
			virtual void onResponse(boost::shared_ptr<RedisResponse> response) {
				MY_LOG4CXX_DEBUG(gLogger,"!! REDIS RESPONSE : Ticket %d", response->ticket());
			}
	};

	class TestClientController : public ClientController {
		public:
			virtual void onConnected() {

				recvedCnt_ = 0;

				for(int i = 0; i < GET_COUNT; i++) {
					char userId[1024] = {0, };
					sprintf(userId, "userid_num%d@naver.com", i);
					int ticket = gRedisCtrl_->get(userId, this);
					(void)ticket;
					//MY_LOG4CXX_INFO(gLogger,"!! >>>>>>>>>>>>>>>>>>>>>>>>>> REQUSET REDIS GET COMMAND : %d = %s", ticket, userId);
				}
			}


			virtual void onControllerEvent_GotResponse(
					boost::shared_ptr<BaseController> controller, 
					int ticket) {
				recvedCnt_ ++;

				MY_LOG4CXX_DEBUG(gLogger,"onControllerEvent_GotResponse emitted.. ticket %d, recvCnt %d\n", ticket, recvedCnt_);
				if(recvedCnt_ == GET_COUNT) {
					MY_LOG4CXX_INFO(gLogger, "************ Test Success ************");
					ioServiceContainer()->stop();	// test success
				}
			}

		private:
			int recvedCnt_;
	};

	class TestServerController : public ServerController {
		virtual boost::shared_ptr<ClientController> onAccept(boost::shared_ptr<TcpSocket> socket) {
			boost::shared_ptr<TestClientController> newController(new TestClientController);
			return newController;
		}
	};


	bool doTest() {
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");
		MY_LOG4CXX_INFO(gLogger, "Redis Request Test");
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");

		boost::shared_ptr<BaseIOServiceContainer> ioServiceContainer;
		ioServiceContainer = boost::shared_ptr<BaseIOServiceContainer>(new IOServiceContainer(2));
		ioServiceContainer->initialize();

		try {
			boost::shared_ptr<TestServerController> serverController(new TestServerController);
			NetworkHelper::listenTcp(ioServiceContainer.get(), gPortBase, serverController);

			// from user.. this user send to below 2 users..
			boost::shared_ptr<TestClientController> clientController(new TestClientController);
			NetworkHelper::connectTcp(ioServiceContainer.get(), "localhost", gPortBase, clientController);

			gRedisCtrl_ = boost::shared_ptr<TestRedisController>(new TestRedisController());
			NetworkHelper::connectRedis(ioServiceContainer.get(), REDIS_ADDRESS, 6379, gRedisCtrl_);

			gPortBase++;

			ioServiceContainer->run();
			MY_LOG4CXX_INFO(gLogger, "Test OK");
			return true;
		} catch(Exception &e) {
			printf("Exception emitted : %s\n", e.what());
		}
		return false;
	}
}

// Flow 
// #. 2 user login
// #. server client receive login packet
// #. set user location to "redis"
// #. server client receive response from redis
// #. send login result to user
// #. increase login ok cnt
// #. wait login ok cnt == 2
// #. send memo to 2 user 
// #. server client receive send memo packet
// #. get user location from "redis"
// #. server client receive response from redis
// #. relay send memo payload to user
// #. stop if recv memo cnt == 2

namespace TestFrameAndStringListAndLineProtocolAndRedis {

	class TestServerClientController;

	static const int COMMAND_LOGIN = 813;
	static const int COMMAND_SEND_MEMO = 923;
	static const int LOGIN_USER_COUNT = 30;
	static const unsigned short TIMER_ID_CHECK_LOGIN_OK = 1;
	static const unsigned short TIMER_ID_CHECK_LOGIN_WAIT_PATIENCE = 2;

	typedef std::map<std::string, boost::shared_ptr<BaseController> > mapUser_t;
	static mapUser_t gUserMap;
	static Mutex gLockMutex;
	static int gLoginUserCnt = 0;
	static int gRecvMemoCnt = 0;

	class TestRedisController : public RedisController {
		public:
			virtual void onResponse(boost::shared_ptr<RedisResponse> response) {
				MY_LOG4CXX_DEBUG(gLogger,"!! REDIS RESPONSE : Ticket %d => %s\n", response->ticket(), response->result()->str.c_str());
			}
	};

	class TestLoginClientController : public FrameController {
		public:
			TestLoginClientController(const std::string &userId) : dummyLoginId(userId) { }

			virtual void onConnected() {
				MY_LOG4CXX_DEBUG(gLogger, "onConnected emitted\n");

				LineProtocol lprot;
				lprot.setLine(dummyLoginId);

				FrameHeader header(COMMAND_LOGIN, 1111);
				writeFrame(header, &lprot);
			}

			virtual void onFrameReceived(boost::shared_ptr<FrameProtocol> prot) {
				MY_LOG4CXX_DEBUG(gLogger, "[CLIENT] onFrameReceived called : %d:%d", prot->header().command(), prot->payloadSize());

				// got response
				switch(prot->header().command()) {
					case COMMAND_LOGIN:
						assert(prot->header().transactionId() == 1111);
						gLockMutex.lock();
						gLoginUserCnt++;
						gLockMutex.unlock();
						break;
					case COMMAND_SEND_MEMO:
						{
							assert(prot->header().transactionId() == 9999);
							LineProtocol lprot(prot);
							lprot.processReadFromPayloadBuffer();
							assert(lprot.isReadComplete());

							gLockMutex.lock();
							gRecvMemoCnt++;
							gLockMutex.unlock();
							MY_LOG4CXX_DEBUG(gLogger, "[CLIENT] recv COMMAND_SEND_MEMO cnt %d / %d\n", gRecvMemoCnt, gLoginUserCnt);
							if(gRecvMemoCnt == gLoginUserCnt) {
								MY_LOG4CXX_INFO(gLogger, "************ Test Success ************");
								ioServiceContainer()->stop();	// test success
							}
						}							
					default:
						break;
				}
			}
		private:
			std::string dummyLoginId;
	};

	
	class TestSendMemoClientController : public FrameController {
		public:
			TestSendMemoClientController() { }

			virtual void onConnected() {
				MY_LOG4CXX_DEBUG(gLogger, "onConnected emitted\n");

				setTimer(TIMER_ID_CHECK_LOGIN_OK, 100, true);
				setTimer(TIMER_ID_CHECK_LOGIN_WAIT_PATIENCE, 2000, false);
			}

			virtual void onTimer(unsigned short id) {
				assert(id == TIMER_ID_CHECK_LOGIN_OK || id == TIMER_ID_CHECK_LOGIN_WAIT_PATIENCE);

				if(id == TIMER_ID_CHECK_LOGIN_WAIT_PATIENCE) {
					if(gLoginUserCnt != LOGIN_USER_COUNT) {
						// run out of patience 
						assert(false && "run out of patience with waiting login");
					}
					return;
				}

				if(gLoginUserCnt == LOGIN_USER_COUNT) {
					MY_LOG4CXX_INFO(gLogger, "--------------- All user login complete! (: -----------------");

					// send memo packet
					StringListProtocol slprot;
					LineProtocol lprot(&slprot);

					for(int i = 0; i < LOGIN_USER_COUNT; i++) {
						char userId[1024] = {0, };
						sprintf(userId, "userid_num%d@naver.com", i);
						slprot.addString(userId);
					}
					lprot.setLine("MEMO : Hello? how do you do??");

					FrameHeader header(COMMAND_SEND_MEMO, 9999);
					writeFrame(header, &lprot);

					killTimer(id);
				}
			}

			virtual void onFrameReceived(boost::shared_ptr<FrameProtocol> prot) {
				MY_LOG4CXX_DEBUG(gLogger, "[CLIENT] onFrameReceived called : %d:%d", prot->header().command(), prot->payloadSize());

				// got response
				switch(prot->header().command()) {
					case COMMAND_SEND_MEMO:
						assert(prot->header().transactionId() == 9999);
						break;
					default:
						break;
				}
			}
	};

	class TestServerClientController : public FrameController {
		public:
			TestServerClientController(boost::shared_ptr<TestRedisController> ctrl) 
					: redisCtrl_(ctrl)
					, recvedRedisResultCnt_(0)
					, ticketLogin_(0) { }

			virtual void onFrameReceived(boost::shared_ptr<FrameProtocol> prot) {
				MY_LOG4CXX_DEBUG(gLogger, "[SERVER] onFrameReceived called : %d:%d", prot->header().command(), prot->payloadSize());

				// got request from client
				switch(prot->header().command()) {
					case COMMAND_LOGIN:
					{
						headerLogin_ = prot->header();

						LineProtocol lprot(prot);
						lprot.processReadFromPayloadBuffer();
						assert(lprot.isReadComplete());

						gLockMutex.lock();
						gUserMap.insert(mapUser_t::value_type(lprot.linePtr(), shared_from_this()));
						gLockMutex.unlock();

						// register location info.
						char value[1024] = {0,};
						sprintf(value, "127.0.0.1:8000, dummyid=%s", lprot.linePtr());// dummy location routing string.. (:
						ticketLogin_ = redisCtrl_->set(lprot.linePtr(), value, this);
						break;
					}
					case COMMAND_SEND_MEMO:
						{
							headerSendMemo_ = prot->header();

							StringListProtocol slprot(prot);
							LineProtocol lprot(&slprot);
							lprot.processReadFromPayloadBuffer();

							assert(strcmp(lprot.linePtr(), "MEMO : Hello? how do you do??") == 0);
							assert((int)slprot.listSize() == LOGIN_USER_COUNT);
							
							payloadSendMemo_ = lprot.linePtr();

							for(size_t i = 0; i < slprot.listSize(); i++) {
								int ticket = redisCtrl_->redisRequest()->get(slprot.stringOf(i).c_str());
								redisCtrl_->eventGotResponse()->registerObserver(ticket, this);
							}
							break;
						}
					default:
						// blah~ blah~ more command code..
						assert(false && "Unknown packet command..");

						break;
				}
			}

			virtual void onControllerEvent_GotResponse(
					boost::shared_ptr<BaseController> controller, 
					int ticket) {
				MY_LOG4CXX_DEBUG(gLogger,"[SERVER] onControllerEvent_GotResponse emitted.. ticket %d\n", ticket);

				boost::shared_ptr<RedisController> redis = REDIS_CTRL(controller); 
				boost::shared_ptr<RedisResponse> res = redis->getAndDeleteResponseOfTicket(ticket);
				MY_LOG4CXX_INFO(gLogger, "REDIS RESULT : %s, ticket %d, %d\n", res->result()->str.c_str(), ticket, ticketLogin_);

				if(ticket == ticketLogin_) {
					// doing login progress..
					// send login result..
					LineProtocol lprot;
					lprot.setLine("LOGIN OK");
					writeFrame(headerLogin_, &lprot);
					return;
				}

				// send to memopacket to this user location!.... 
				// but here is not real world. stop dreaming! 
#define TOKEN "dummyid="
				const char *find = strstr(res->result()->str.c_str(), TOKEN);
				assert(find);
				const char *userId = find + strlen(TOKEN);

				gLockMutex.lock();
				mapUser_t::iterator it = gUserMap.find(userId);
				if(it == gUserMap.end()) {
					MY_LOG4CXX_FATAL(gLogger, "gUserMap not found user session. %s, %s, %d\n", userId, res->result()->str.c_str(), gUserMap.size());
					assert(false && "gUserMap not found user session");
				}
				gLockMutex.unlock();

				LineProtocol lprot;
				lprot.setLine(payloadSendMemo_);
				boost::shared_ptr<FrameController> frameCtrl = boost::static_pointer_cast<FrameController>(it->second);
				frameCtrl->writeFrame(headerSendMemo_, &lprot);
			}

		private:
			boost::shared_ptr<TestRedisController> redisCtrl_;
			int recvedRedisResultCnt_;
			int ticketLogin_;
			FrameHeader headerLogin_;
			FrameHeader headerSendMemo_;
			std::string payloadSendMemo_;
	};

	class TestServerController : public ServerController {
		virtual void onInitialized() {
			redisCtrl_ = boost::shared_ptr<TestRedisController>(new TestRedisController());
			NetworkHelper::connectRedis(ioServiceContainer(), REDIS_ADDRESS, 6379, redisCtrl_);
		}
		virtual boost::shared_ptr<ClientController> onAccept(boost::shared_ptr<TcpSocket> socket) {
			boost::shared_ptr<TestServerClientController> newController(new TestServerClientController(redisCtrl_)); 
			return newController;
		}
		private:
			boost::shared_ptr<TestRedisController> redisCtrl_;
	};

	bool doTest() {
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");
		MY_LOG4CXX_INFO(gLogger, "Frame And StringList And Line Protocol And RedisRequestController Test");
		MY_LOG4CXX_INFO(gLogger, "=====================================================================");

		boost::shared_ptr<BaseIOServiceContainer> ioServiceContainer;
		ioServiceContainer = boost::shared_ptr<BaseIOServiceContainer>(new IOServiceContainer(2));
		//ioServiceContainer = boost::shared_ptr<BaseIOServiceContainer>(new IOServiceContainer);
		ioServiceContainer->initialize();

		try {
			boost::shared_ptr<TestServerController> serverController(new TestServerController);
			NetworkHelper::listenTcp(ioServiceContainer.get(), gPortBase, serverController);

			// from user.. this user send to below 2 users..
			boost::shared_ptr<TestSendMemoClientController> clientController(new TestSendMemoClientController);
			NetworkHelper::connectTcp(ioServiceContainer.get(), "localhost", gPortBase, clientController);

			// to user.. dummy logged-in user client.
			std::vector<boost::shared_ptr<BaseController> > clients;
			for(int i = 0; i < LOGIN_USER_COUNT; i++) {
				char userId[1024] = {0, };
				sprintf(userId, "userid_num%d@naver.com", i);
				boost::shared_ptr<TestLoginClientController> loginUser(new TestLoginClientController(userId));
				NetworkHelper::connectTcp(ioServiceContainer.get(), "localhost", gPortBase, loginUser);

				clients.push_back(loginUser);
			}

			gPortBase++;

			ioServiceContainer->run();
			MY_LOG4CXX_INFO(gLogger, "Test OK");
			return true;
		} catch(Exception &e) {
			printf("Exception emitted : %s\n", e.what());
		}
		return false;
	}
}

void coconutLog(logger::LogLevel level, const char *fileName, int fileLine, const char *functionName, const char *logmsg) {
	printf("[COCONUT] <%d> %s\n", level, logmsg);
}

int main() {
#if defined(USE_LOG4CXX)
	PropertyConfigurator::configure("log4cxx.properties");
#endif

//#define SHOW_COCONUT_LOG
#if defined(SHOW_COCONUT_LOG) || !defined(USE_LOG4CXX)
	logger::LogHookCallback logCallback;
	logCallback.trace = coconutLog;
	logCallback.debug = coconutLog;
	logCallback.info = coconutLog;
	logCallback.warning = coconutLog;
	logCallback.error = coconutLog;
	logCallback.fatal = coconutLog;
	logger::setLogHookFunctionCallback(logCallback);
#endif
	logger::setLogLevel(logger::LEVEL_TRACE);
	//logger::setLogLevel(logger::LEVEL_INFO);

	MY_LOG4CXX_INFO(gLogger, "Entering protocol test");

	assert(TestUDPAndLineProtocol::doTest());
	assert(TestHttpRequestGet::doTest());
	assert(TestLineProtocol::doTest());
	assert(TestFrameProtocol::doTest());
	assert(TestFrameAndStringListProtocol::doTest());
	assert(TestFrameAndStringListAndLineProtocol::doTest());
#if ! defined(WIN32)
	assert(TestFileDescriptorProtocol::doTest());
#endif
	assert(TestFrameAndStringListAndLineProtocolAndRedis::doTest());
	assert(TestRedisRequest::doTest());

	MY_LOG4CXX_INFO(gLogger, "Leaving protocol test");

#if defined(WIN32)
	_getch();
#endif
	return 0;
}
