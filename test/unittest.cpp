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
#include <assert.h>
#if defined(WIN32)
#include <conio.h>
#endif
#include <fcntl.h>
#include "NetworkHelper.h"
#include "IOService.h"
#include "IOServiceContainer.h"
#include "LineController.h"
#include "JSONController.h"
#include "FrameController.h"
#include "JSONProtocol.h"
#include "HttpClient.h"
#include "RedisRequest.h"
#include "PlaceHolders.h"
#include "FileDescriptorController.h"
#include "Logger.h"

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
			LOG_INFO("TestUdpClientController LINE RECEIVED : [%s] port : %d\n", 
					line, ntohs(udpSocket()->lastClientAddress()->sin_port));

			LOG_INFO("************ Test Success ************");
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
			LOG_INFO("TestUdpServerController LINE RECEIVED : [%s]\n", line);
			writeLine(line);
		}
	};

	bool doTest() {
		LOG_INFO("=====================================================================");
		LOG_INFO("UDP And Line Protocol Test");
		LOG_INFO("=====================================================================");

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


namespace TestHttpClientGet
{
	class TestHttpController : public coconut::HttpClient::EventHandler
	{
		virtual void onHttpClient_ReceivedChunked(HttpClient *client, int receivedsize) { 
			LOG_INFO("TestHttpController received size : %d byte\n", receivedsize);
		}

		virtual void onHttpClient_Error(HttpClient *client, coconut::HttpClient::ErrorCode errorcode) {
			LOG_INFO("TestHttpController onError : %d\n", errorcode);
		}

		virtual void onHttpClient_Response(HttpClient *client, int rescode) {
			LOG_INFO("TestHttpController onResponse [this = %p], rescode = %d, size = %d\n", 
				this, rescode, client->responseBodySize());

			printf("%s\n", (char *)client->responseBody());

			LOG_INFO("************ Test Success ************");
			client->ioService()->ioServiceContainer()->stop();	// test success
		}
	};

	bool doTest() {
		LOG_INFO("=====================================================================");
		LOG_INFO("Http Request GET Test");
		LOG_INFO("=====================================================================");

		IOServiceContainer ioServiceContainer;
		ioServiceContainer.initialize();

		try {
			std::string uri;
			uri = "http://119.205.238.162:8081/test.php";
			TestHttpController controller;

			HttpClient client(ioServiceContainer.ioServiceByRoundRobin());
			client.setEventHandler(&controller);
			client.request(coconut::HTTP_POST, uri.c_str(), NULL, 20);

			ioServiceContainer.run();
			return true;
		} catch(Exception &e) {
			LOG_FATAL("Exception emitted : %s\n", e.what());
		}
		return false;
	}

}


namespace TestLineProtocol {

	class TestClientController : public LineController {
		virtual void onConnected() {
			LOG_DEBUG("onConnected called : %d", socket()->socketFD());
			recvedLine_ = 0;
			socket()->write("HELLO\r\n", 7);
			socket()->write("HELLO\r\n", 7);
		}

		virtual void onLineReceived(const char *line) {
			LOG_DEBUG("[CLIENT] onLineReceived called : %s", line);

			if(strcmp(line, "How's it going?") == 0) {
				recvedLine_++;
				if(recvedLine_ == 2) {
					LOG_INFO("************ Test Success ************");
					ioServiceContainer()->stop();	// test success
				}
			}
		}

		private:
		int recvedLine_;
	};

	class TestServerClientController : public LineController {
		virtual void onInitialized() {
			LOG_DEBUG("onInitialized called : %d", socket()->socketFD());
		}
		virtual void onLineReceived(const char *line) {
			LOG_DEBUG("[SERVER] onLineReceived called : %s", line);
			writeLine("How's it going?");
		}
	};

	class TestServerController : public ServerController {

		virtual boost::shared_ptr<ClientController> onAccept(boost::shared_ptr<TcpSocket> socket) {
			
			boost::shared_ptr<TestServerClientController> newController(new TestServerClientController); 
			LOG_DEBUG("[SERVER] onAccept called : %p", newController.get());
			return newController;
		}
	};

	bool doTest() {
		LOG_INFO("=====================================================================");
		LOG_INFO("Line Protocol Test");
		LOG_INFO("=====================================================================");

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


namespace TestJSONProtocol {
	static const char *JSON_STR = "{\"RootA\":\"Value in parent\",\"ChildNode\":{\"ChildA\":\"String Value\",\"ChildB\":\"42\"}}";

	class TestClientController : public JSONController {
		virtual void onConnected() {
			LOG_DEBUG("onConnected called : %d", socket()->socketFD());
			writeJSON(JSON_STR);
			writeJSON(JSON_STR);
			recvedLine_ = 0;
		}

		virtual void onJSONReceived(const char *json) {
			LOG_INFO("[SERVER] onJSONReceived called : %s", json);

			assert(strcmp(JSON_STR, json) == 0);

			recvedLine_++;
			if(recvedLine_ == 2) {
				LOG_INFO("************ Test Success ************");
				ioServiceContainer()->stop();	// test success
			}
		}

		private:
		int recvedLine_;
	};

	class TestServerClientController : public JSONController {
		virtual void onInitialized() {
			LOG_DEBUG("onInitialized called : %d", socket()->socketFD());
		}
		virtual void onJSONReceived(const char *json) {
			LOG_DEBUG("[SERVER] onJSONReceived called : %s", json);
			writeJSON(json);
		}
	};

	class TestServerController : public ServerController {

		virtual boost::shared_ptr<ClientController> onAccept(boost::shared_ptr<TcpSocket> socket) {
			
			boost::shared_ptr<TestServerClientController> newController(new TestServerClientController); 
			LOG_DEBUG("[SERVER] onAccept called : %p", newController.get());
			return newController;
		}
	};

	bool doTest() {
		LOG_INFO("=====================================================================");
		LOG_INFO("JSON Protocol Test");
		LOG_INFO("=====================================================================");

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
			LOG_DEBUG("onConnected called");
			recvedLine_ = 0;

			FrameHeader header(COMMAND, 0);
			writeFrame(header, "0PAYLOAD", 8);	
			writeFrame(header, "1PAYLOAD", 8);	
			writeFrame(header, "2PAYLOAD", 8);	
			writeFrame(header, "3PAYLOAD", 8);	
			writeFrame(header, "4PAYLOAD", 8);	
		}

		virtual void onFrameReceived(boost::shared_ptr<FrameProtocol> prot) {
			LOG_DEBUG("[CLIENT] onFrameReceived called : %d %s:%d", prot->header().command(), prot->payloadPtr(), prot->payloadSize());
			
			std::string payload;
			payload.assign((char *)prot->payloadPtr(), prot->payloadSize());

			char expactedPayload[1024] = {0, };
			sprintf(expactedPayload, "RESPOSE : %dPAYLOAD", recvedLine_);

			assert(COMMAND + 1 == prot->header().command());
			assert(strcmp(expactedPayload, payload.c_str()) == 0);

			recvedLine_++;
			if(recvedLine_ == 5) {
				LOG_INFO("************ Test Success ************");
				ioServiceContainer()->stop();	// test success
			}
		}

		private:
		int recvedLine_;
	};

	class TestServerClientController : public FrameController {
		virtual void onFrameReceived(boost::shared_ptr<FrameProtocol> prot) {
			LOG_DEBUG("[SERVER] onFrameReceived called : %d %s:%d", prot->header().command(), prot->payloadPtr(), prot->payloadSize());
			
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
		LOG_INFO("=====================================================================");
		LOG_INFO("Frame Protocol Test");
		LOG_INFO("=====================================================================");

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


namespace TestFrameAndJSONProtocol {
	static const int COMMAND = 813;
	static const char *JSON_STR = "{\"RootA\":\"Value in parent\",\"ChildNode\":{\"ChildA\":\"String Value\",\"ChildB\":\"42\"}}";

	class TestClientController : public FrameController {

		virtual void onConnected() {
			LOG_DEBUG("onConnected called");
			recvedLine_ = 0;

			boost::shared_ptr<JSONProtocol> jsonprot(new JSONProtocol);
			jsonprot->setJSON(JSON_STR);
			jsonprot->processSerialize();

			FrameHeader header(COMMAND, 0);
			writeFrame(header, jsonprot->writingBuffer());
			writeFrame(header, jsonprot->writingBuffer());
		}

		virtual void onFrameReceived(boost::shared_ptr<FrameProtocol> prot) {
			LOG_DEBUG("[CLIENT] onFrameReceived called : %d:%d", prot->header().command(), prot->payloadSize());
			
			boost::shared_ptr<JSONProtocol> jsonprot(new JSONProtocol(prot));
			jsonprot->processReadFromPayloadBuffer();
			assert(jsonprot->isReadComplete());
			LOG_DEBUG("[CLIENT] json = %s", jsonprot->jsonPtr());

			assert(strcmp(JSON_STR, jsonprot->jsonPtr()) == 0);

			recvedLine_++;
			if(recvedLine_ == 2) {
				LOG_INFO("************ Test Success ************");
				ioServiceContainer()->stop();	// test success
			}
		}

		private:
		int recvedLine_;
	};

	class TestServerClientController : public FrameController {
		virtual void onFrameReceived(boost::shared_ptr<FrameProtocol> prot) {
			LOG_DEBUG("[SERVER] onFrameReceived called : %d:%d", prot->header().command(), prot->payloadSize());

			boost::shared_ptr<JSONProtocol> jsonprot(new JSONProtocol(prot));
			jsonprot->processReadFromPayloadBuffer();

			assert(jsonprot->isReadComplete());

			jsonprot->processSerialize();
			FrameHeader header(prot->header().command() + 1, 2);
			writeFrame(header, jsonprot->writingBuffer());
		}
	};

	class TestServerController : public ServerController {
		virtual boost::shared_ptr<ClientController> onAccept(boost::shared_ptr<TcpSocket> socket) {
			boost::shared_ptr<TestServerClientController> newController(new TestServerClientController); 
			return newController;
		}
	};

	bool doTest() {
		LOG_INFO("=====================================================================");
		LOG_INFO("Frame And JSON Protocol Test");
		LOG_INFO("=====================================================================");

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


namespace TestFrameAndStringListProtocol {
	static const int COMMAND = 813;

	class TestClientController : public FrameController {

		virtual void onConnected() {
			LOG_DEBUG("onConnected called");
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
			LOG_DEBUG("[CLIENT] onFrameReceived called : %d:%d", prot->header().command(), prot->payloadSize());
			
			boost::shared_ptr<StringListProtocol> slprot(new StringListProtocol(prot));
			slprot->processReadFromPayloadBuffer();

			assert(slprot->isReadComplete());
			for(size_t i = 0; i < slprot->listSize(); i++) {
				LOG_DEBUG("[CLIENT] list string [%d] = %s\n", i, slprot->stringOf(i).c_str());
			}

			assert(slprot->listSize() == 4);
			assert(slprot->stringOf(0) == "ID1");
			assert(slprot->stringOf(1) == "ID2");
			assert(slprot->stringOf(2) == "ID3");
			assert(slprot->stringOf(3) == "NEW ID4 FROM SERVER");
			recvedLine_++;
			if(recvedLine_ == 2) {
				LOG_INFO("************ Test Success ************");
				ioServiceContainer()->stop();	// test success
			}
		}

		private:
		int recvedLine_;
	};

	class TestServerClientController : public FrameController {
		virtual void onFrameReceived(boost::shared_ptr<FrameProtocol> prot) {
			LOG_DEBUG("[SERVER] onFrameReceived called : %d:%d", prot->header().command(), prot->payloadSize());

			boost::shared_ptr<StringListProtocol> slprot(new StringListProtocol(prot));
			slprot->processReadFromPayloadBuffer();

			assert(slprot->isReadComplete());
			assert(slprot->listSize() == 3);
			assert(slprot->stringOf(0) == "ID1");
			assert(slprot->stringOf(1) == "ID2");
			assert(slprot->stringOf(2) == "ID3");
			for(size_t i = 0; i < slprot->listSize(); i++) {
				LOG_DEBUG("[SERVER] list string [%d] = %s\n", i, slprot->stringOf(i).c_str());
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
		LOG_INFO("=====================================================================");
		LOG_INFO("Frame And String List Protocol Test");
		LOG_INFO("=====================================================================");

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
			LOG_DEBUG("onConnected called : %p", this);
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
			LOG_DEBUG("[CLIENT] onFrameReceived called : %d:%d", prot->header().command(), prot->payloadSize());
			
			boost::shared_ptr<StringListProtocol> slprot(new StringListProtocol(prot));
			boost::shared_ptr<LineProtocol> lprot(new LineProtocol(slprot));
			lprot->processReadFromPayloadBuffer();

			assert(lprot->isReadComplete());
			for(size_t i = 0; i < slprot->listSize(); i++) {
				LOG_DEBUG("[CLIENT] list string [%d] = %s\n", i, slprot->stringOf(i).c_str());
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
				LOG_INFO("************ Test Success ************");
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
			LOG_DEBUG("[SERVER] onFrameReceived called : %d:%d", prot->header().command(), prot->payloadSize());

			StringListProtocol slprot(prot.get());
			LineProtocol lprot(&slprot);

			lprot.processReadFromPayloadBuffer();
			
			assert(lprot.isReadComplete());
			for(size_t i = 0; i < slprot.listSize(); i++) {
				LOG_DEBUG("[SERVER] list string [%d] = %s\n", i, slprot.stringOf(i).c_str());
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
				LOG_DEBUG("[SERVER] write mode == 0");
				lprot.processWrite(socket());
			} else {
				LOG_DEBUG("[SERVER] write mode == 1");
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
		LOG_INFO("=====================================================================");
		LOG_INFO("Frame And String List And Line Protocol Test");
		LOG_INFO("=====================================================================");

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
				LOG_DEBUG("TestFDController onInitialized emitted.. %d -> %d\n", socket()->socketFD(), fd_);

				if(fd_ > 0)
					writeDescriptor(fd_);
			}
			virtual void onDescriptorReceived(int fd) {
				LOG_DEBUG("TestFDController onDescriptorReceived emitted.. %d\n", fd);

				if(fd <= 0)
					return;

				char buff[1024] = {0, };
				while(true) {
					if(read(fd, buff, sizeof(buff)) > 0) {
						LOG_DEBUG("%s", buff);
						continue;
					}
					break;
				}

				assert(strstr(buff, "TEST READ FILE"));
				assert(strstr(buff, "// END"));

				close(fd);
				LOG_INFO("************ Test Success ************");
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
				LOG_DEBUG("TestUnixServerController onInitialized file descriptor = %d\n", fd_);
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
		LOG_INFO("=====================================================================");
		LOG_INFO("FileDescriptorProtocol Test");
		LOG_INFO("=====================================================================");

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
	static const int TEST_COUNT = 100;

	boost::shared_ptr<RedisRequest> gRedisCtrl_;

	typedef struct testStruct {
		int type;
		int port;
		char address[256];
	} testStruct;

	class TestClientController : public ClientController {
		public:
			virtual void onConnected() {
				recvedCnt_ = 0;

				setCommand();

				// for reserved command test
				gRedisCtrl_->connect();
			}

			void setCommand() {
				testStruct data;
				data.type = 1;
				data.port = 6389;
				strcpy(data.address, "localhost");

				char userId[1024] = {0, };
				sprintf(userId, "userid_num%d@naver.com", recvedCnt_);

				std::string arg;
				arg.assign((char *)&data, sizeof(data));

				std::vector<std::string> args;
				args.push_back(userId);
				args.push_back(arg);
				gRedisCtrl_->command("SET", args, 
											boost::bind(&TestClientController::onRedisRequest_Response, 
												this, 
												coconut::placeholders::redisResponse)
										);
				expactedRecvSetCommand_ = true;
			}

			void getCommand() {
				char userId[1024] = {0, };
				sprintf(userId, "userid_num%d@naver.com", recvedCnt_);

				std::vector<std::string> args;
				args.push_back(userId);

				gRedisCtrl_->command("GET", args, 
											boost::bind(&TestClientController::onRedisRequest_Response, 
												this, 
												coconut::placeholders::redisResponse)
										);
	
				expactedRecvSetCommand_ = false;
			}

			virtual void onRedisRequest_Response(boost::shared_ptr<RedisResponse> response) { 
				LOG_DEBUG("onRedisRequest_Response emitted.. ticket %d, data %s, recvCnt %d\n", 
					response->ticket(), response->resultData()->strValue.c_str(), recvedCnt_);

				if(expactedRecvSetCommand_) {
					getCommand();
				} else {
					testStruct getData;
					memcpy(&getData, (void *)response->resultData()->strValue.c_str(), response->resultData()->strValue.size());

					assert(getData.type == 1);
					assert(getData.port == 6389);
					assert(strcmp(getData.address, "localhost") == 0);

					recvedCnt_++;

					if(recvedCnt_ == TEST_COUNT) {
						LOG_INFO("************ Test Success ************");
						ioServiceContainer()->stop();	// test success
					} else {
						setCommand();
					}
				}
			}

		private:
			int recvedCnt_;
			bool expactedRecvSetCommand_; // toggled set <-> get command
	};

	class TestServerController : public ServerController {
		virtual boost::shared_ptr<ClientController> onAccept(boost::shared_ptr<TcpSocket> socket) {
			boost::shared_ptr<TestClientController> newController(new TestClientController);
			return newController;
		}
	};

	bool doTest() {
		LOG_INFO("=====================================================================");
		LOG_INFO("Redis Request Test");
		LOG_INFO("=====================================================================");

		boost::shared_ptr<BaseIOServiceContainer> ioServiceContainer;
		ioServiceContainer = boost::shared_ptr<BaseIOServiceContainer>(new IOServiceContainer(2));
		ioServiceContainer->initialize();

		try {
			boost::shared_ptr<TestServerController> serverController(new TestServerController);
			NetworkHelper::listenTcp(ioServiceContainer.get(), gPortBase, serverController);

			boost::shared_ptr<TestClientController> clientController(new TestClientController);
			NetworkHelper::connectTcp(ioServiceContainer.get(), "localhost", gPortBase, clientController);

			gRedisCtrl_ = boost::shared_ptr<RedisRequest>(new RedisRequest(ioServiceContainer->ioServiceByRoundRobin(), 
																	REDIS_ADDRESS, 6379));

			gPortBase++;

			ioServiceContainer->run();
			LOG_INFO("Test OK");
			return true;
		} catch(Exception &e) {
			printf("Exception emitted : %s\n", e.what());
		}
		return false;
	}
}

// Flow 
// #. {LOGIN_USER_COUNT} user login
// #. server client receive login packet
// #. set user location to "redis"
// #. server client receive response from redis
// #. send login result to user
// #. increase login ok cnt
// #. wait login ok cnt == {LOGIN_USER_COUNT}
// #. send memo to {LOGIN_USER_COUNT} user 
// #. server client receive send memo packet
// #. get user location from "redis"
// #. server client receive response from redis
// #. relay send memo payload to user
// #. stop if recv memo cnt == {LOGIN_USER_COUNT}

namespace TestFrameAndStringListAndLineProtocolAndRedis {

	class TestServerClientController;

	static const int COMMAND_LOGIN = 813;
	static const int COMMAND_SEND_MEMO = 923;
	static const int LOGIN_USER_COUNT = 120;
	static const int WAIT_PATIENCE_MSEC = LOGIN_USER_COUNT * 50;
	static const unsigned short TIMER_ID_CHECK_LOGIN_OK = 1;
	static const unsigned short TIMER_ID_CHECK_LOGIN_WAIT_PATIENCE = 2;

	typedef std::map<std::string, boost::shared_ptr<BaseController> > mapUser_t;
	static mapUser_t gUserMap;
	static Mutex gLockMutex;
	static int gLoginUserCnt = 0;
	static int gRecvMemoCnt = 0;
	static int gErrorCnt = 0;

	class TestLoginClientController : public FrameController {
		public:
			TestLoginClientController(const std::string &userId) : dummyLoginId(userId) { }

			virtual void onConnected() {
				LOG_TRACE("[CLIENT] TestLoginClientController::onConnected emitted\n");

				LineProtocol lprot;
				lprot.setLine(dummyLoginId);

				FrameHeader header(COMMAND_LOGIN, 1111);
				writeFrame(header, &lprot);
			}

			void onError(int error, const char*strerror) {

				gLockMutex.lock();
				gErrorCnt++;
				gLockMutex.unlock();

				LOG_ERROR("[CLIENT] TestLoginClientController::onSocket_Error emitted : %s, total error cnt : %d\n", 
							strerror, gErrorCnt);
			}

			virtual void onFrameReceived(boost::shared_ptr<FrameProtocol> prot) {
				LOG_DEBUG("[CLIENT] TestLoginClientController::onFrameReceived called : %d:%d, loginUser : %d", 
						prot->header().command(), prot->payloadSize(), gLoginUserCnt);

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
							LOG_DEBUG("[CLIENT] recv COMMAND_SEND_MEMO cnt %d / %d\n", gRecvMemoCnt, gLoginUserCnt);
							if(gRecvMemoCnt == gLoginUserCnt) {
								LOG_INFO("************ Test Success ************");
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
				LOG_DEBUG("onConnected emitted\n");

				setTimer(TIMER_ID_CHECK_LOGIN_OK, 100, true);
				setTimer(TIMER_ID_CHECK_LOGIN_WAIT_PATIENCE, WAIT_PATIENCE_MSEC, false);
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
					LOG_INFO("######### All user login complete! (: #########");

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
				LOG_DEBUG("[CLIENT] TestSendMemoClientController onFrameReceived called : %d:%d", prot->header().command(), prot->payloadSize());

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
			TestServerClientController(boost::shared_ptr<RedisRequest> request) 
					: redisRequest_(request)
					, recvedRedisResultCnt_(0)
					, ticketLogin_(0) { }

			virtual void onFrameReceived(boost::shared_ptr<FrameProtocol> prot) {
				LOG_DEBUG("[SERVER] onFrameReceived called : %d:%d", prot->header().command(), prot->payloadSize());

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

						std::vector<std::string> args;
						args.push_back(lprot.linePtr());
						args.push_back(value);
						ticketLogin_ = redisRequest_->command("SET", args, 
													boost::bind(&TestServerClientController::onRedisRequest_Response, 
														this, 
														coconut::placeholders::redisResponse)
												);
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
								std::vector<std::string> args;
								args.push_back(slprot.stringOf(i));

								redisRequest_->command("GET", args, 
													boost::bind(&TestServerClientController::onRedisRequest_Response, 
														this, 
														coconut::placeholders::redisResponse)
												);
							}
							break;
						}
					default:
						// blah~ blah~ more command code..
						assert(false && "Unknown packet command..");

						break;
				}
			}

			virtual void onRedisRequest_Response(boost::shared_ptr<RedisResponse> response) { 
				LOG_DEBUG("[SERVER] REDIS RESULT : %s, ticket %d, %d, loginUser : %d\n", 
						response->resultData()->strValue.c_str(), response->ticket(), ticketLogin_, loginOKCnt_);

				if(response->ticket() == ticketLogin_) {
					// doing login progress..
					// send login result..
					LineProtocol lprot;
					lprot.setLine("LOGIN OK");
					writeFrame(headerLogin_, &lprot);
					coconut::atomicIncreaseInt32(&loginOKCnt_);
					return;
				}

				// send to memopacket to this user location!
				// but here is not real world. stop dreaming! 
#define TOKEN "dummyid="
				const char *find = strstr(response->resultData()->strValue.c_str(), TOKEN);
				assert(find);
				const char *userId = find + strlen(TOKEN);

				gLockMutex.lock();
				mapUser_t::iterator it = gUserMap.find(userId);
				if(it == gUserMap.end()) {
					LOG_FATAL("gUserMap not found user session. %s, %s, %d\n", 
							userId, response->resultData()->strValue.c_str(), gUserMap.size());
					assert(false && "gUserMap not found user session");
				}
				gLockMutex.unlock();

				// send memo to that user..
				LineProtocol lprot;
				lprot.setLine(payloadSendMemo_);
				boost::shared_ptr<FrameController> frameCtrl = boost::static_pointer_cast<FrameController>(it->second);
				frameCtrl->writeFrame(headerSendMemo_, &lprot);
			}

		private:
			boost::shared_ptr<RedisRequest> redisRequest_;
			int recvedRedisResultCnt_;
			static volatile boost::uint32_t loginOKCnt_;
			int ticketLogin_;
			FrameHeader headerLogin_;
			FrameHeader headerSendMemo_;
			std::string payloadSendMemo_;
	};
	 volatile boost::uint32_t TestServerClientController::loginOKCnt_;


	class TestServerController : public ServerController {
		virtual void onInitialized() {
			redisRequest_ = boost::shared_ptr<RedisRequest>(new RedisRequest(
																ioServiceContainer()->ioServiceByRoundRobin(), 
																REDIS_ADDRESS, 6379
                                                           ));
			redisRequest_->connect();
		}
		virtual boost::shared_ptr<ClientController> onAccept(boost::shared_ptr<TcpSocket> socket) {
			boost::shared_ptr<TestServerClientController> newController(new TestServerClientController(redisRequest_)); 
			return newController;
		}
		private:
			boost::shared_ptr<RedisRequest> redisRequest_;
	};

	bool doTest() {
		LOG_INFO("=====================================================================");
		LOG_INFO("Frame And StringList And Line Protocol And RedisRequestController Test");
		LOG_INFO("=====================================================================");

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
			LOG_INFO("Test OK");
			return true;
		} catch(Exception &e) {
			printf("Exception emitted : %s\n", e.what());
		}
		return false;
	}
}

void coconutLog(logger::LogLevel level, const char *fileName, int fileLine, const char *functionName, const char *logmsg, bool internalLog) {
	printf("[COCONUT] <%d> %s\n", level, logmsg);
}

int main(int argc, char **argv) {
#define SHOW_COCONUT_LOG
#if defined(SHOW_COCONUT_LOG)
	logger::LogHookCallback logCallback;
	logCallback.trace = coconutLog;
	logCallback.debug = coconutLog;
	logCallback.info = coconutLog;
	logCallback.warning = coconutLog;
	logCallback.error = coconutLog;
	logCallback.fatal = coconutLog;
	logger::setLogHookFunctionCallback(logCallback);
#endif

	logger::setLogLevel(logger::LEVEL_INFO);
	if(argc > 1) {
		logger::setLogLevel((logger::LogLevel)atoi(argv[1]));
	}

	LOG_INFO("Entering protocol test");
	assert(TestRedisRequest::doTest());
	assert(TestUDPAndLineProtocol::doTest());
	assert(TestHttpClientGet::doTest());
	assert(TestLineProtocol::doTest());
	assert(TestJSONProtocol::doTest());
	assert(TestFrameProtocol::doTest());
	assert(TestFrameAndJSONProtocol::doTest());
	assert(TestFrameAndStringListProtocol::doTest());
	assert(TestFrameAndStringListAndLineProtocol::doTest());
#if ! defined(WIN32)
	assert(TestFileDescriptorProtocol::doTest());
#endif
	assert(TestFrameAndStringListAndLineProtocolAndRedis::doTest());

	LOG_INFO("Leaving protocol test");

#if defined(WIN32)
	_getch();
#endif
	return 0;
}
