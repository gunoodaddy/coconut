#include "Coconut.h"
#include <fcntl.h>
#include "NetworkHelper.h"
#include "LineController.h"
#include "FileDescriptorController.h"
#include "FrameProtocol.h"
#include "IOServiceContainer.h"

using namespace coconut::protocol;

#define UDP_PORT	7777
#define BIND_PORT	8765

#define TEST_TCP
#define TEST_TCP_CLIENT
#define TEST_TCP_CLIENT_COUNT	1
//#define TEST_UDP
//#define TEST_UDP_CLIENT
#define TEST_UDS
//#define TEST_HTTP
#define TEST_REDIS

//==========================================================
class MyRedisController : public coconut::RedisController {
public:
	virtual void onConnected() {
	}

	static boost::shared_ptr<coconut::RedisController> instanceOfSingleton(coconut::BaseIOServiceContainer *ioServiceContainer = NULL) {
		static bool s_init = false;
		static boost::shared_ptr<coconut::RedisController> s_instance;
		if(false == s_init) {
			s_instance = boost::shared_ptr<coconut::RedisController>(new MyRedisController());
			coconut::NetworkHelper::connectRedis(ioServiceContainer, "localhost", 6379, s_instance);
			s_init = true;
		}
		return s_instance;
	}

	virtual void onResponse(boost::shared_ptr<coconut::RedisResponse> response) {
		LOG_INFO("!! REDIS RESPONSE : Ticket %d => %s\n", response->ticket(), response->result()->str.c_str());
	}
};


//==========================================================
class MyUdpClientController : public coconut::LineController {
public:
	virtual void onInitialized() {
		setTimer(1, 1000, false);
	}

	/*
	void onReceivedDatagram(const void *data, int size, const struct sockaddr_in *sin) {
		LOG_DEBUG( "onReadFrom emitted.. %d from %s:%d\n", size, inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));
	}
	*/

	virtual void onLineReceived(const char *line) {
		LOG_DEBUG( "MyUdpClientController LINE RECEIVED : [%s]\n", line);
		//writeLine(line);
		socket()->write(line, strlen(line));
	}

	virtual void onTimer(unsigned short id) {

#ifdef TEST_UDP
		// udp client test
		udpSocket()->writeTo("UDP HELLO\r\n", 11, "localhost", UDP_PORT);
#endif
	}
};


//==========================================================
class MyHttpController : public coconut::HttpClientController
{
	virtual void onReceivedChucked(int receivedsize) { 
		LOG_DEBUG( "MyHttpController received size : %d byte\n", receivedsize);
	}

	virtual void onError(coconut::HttpClient::ErrorCode errorcode) {
		LOG_DEBUG( "MyHttpController onError : %d\n", errorcode);
	}

	virtual void onResponse(int rescode) {
		LOG_INFO("MyHttpController onResponse [this = %p]\n", this);
		LOG_INFO("Header [%s]\n", httpClient()->findHeader("profimg-name-base64").c_str());

		int fd = ::open("result", O_CREAT | O_TRUNC | O_WRONLY, 00644);
		int len = write(fd, httpClient()->responseBody(), httpClient()->responseBodySize());
		LOG_INFO("HTTP RESULT SAVE FILE : %d bytes\n", len);
		close(fd);

//		ioServiceContainer()->stop();
	}
};


//==========================================================
class MyClientController : public coconut::LineController {
public:
	MyClientController(int id) : id_(id) {
	}
	virtual void onLineReceived(const char *line) {
		LOG_DEBUG( "[%d] MyClientController LINE RECEIVED : [%s]\n", id_, line);
//		socket()->write(line, strlen(line));
//		socket()->write("\r\n", 2);
	}

	virtual void onError(int error, const char *strerror) {
		LOG_DEBUG( "[%d] MyClientController onError emitted.. %d %s, thread = %p, socket = %p\n", 
				id_, error, strerror, (void*)pthread_self(), socket().get());
	}

	virtual void onClosed() { 
		LOG_DEBUG( "[%d] MyClientController onClose emitted..\n", id_);
	}

	virtual void onConnected() {
		LOG_DEBUG( "[%d] MyClientController onConnected emitted.. thread = %p\n", id_, (void *)pthread_self());
		socket()->write((const void *)"HELLO\r\n", 7);

#ifdef TEST_REDIS
		ticket_ = MyRedisController::instanceOfSingleton(ioServiceContainer())->redisRequest()->get("keh");
		MyRedisController::instanceOfSingleton(ioServiceContainer())->eventGotResponse()->registerObserver(ticket_, this);
#endif

#ifdef TEST_HTTP
		std::string uri = "http://119.205.238.162:8081/test.php?arg=0&arg2=2&userid='tester@hangame.com'";
		boost::shared_ptr<MyHttpController> httpController(new MyHttpController);
		coconut::NetworkHelper::httpClient(ioServiceContainer(), coconut::HTTP_POST, uri.c_str(), 20, NULL, httpController);

		httpController_ = httpController;
		httpController->eventGotResponse()->registerObserver(0, this);
#endif
	}

	virtual void onControllerEvent_GotResponse(
						boost::shared_ptr<coconut::BaseController> controller, 
						int ticket) {
		LOG_INFO("[%d] MyClientController onControllerEvent_GotResponse emitted.. ticket %d\n", id_, ticket);

#ifdef TEST_HTTP
		if(httpController_.get() == controller.get()) {
			boost::shared_ptr<MyHttpController> d = boost::static_pointer_cast<MyHttpController>(controller); 
			LOG_DEBUG( "d.responseBody() ==>[this = %p]\n%s\n", controller.get(), (char *)d->httpClient()->responseBody());
		}
			// for close test
			//setTimer(1024, 1000, false);
#endif

#ifdef TEST_REDIS
		if(MyRedisController::instanceOfSingleton().get() == controller.get()) {
			try {
				boost::shared_ptr<coconut::RedisController> d = boost::static_pointer_cast<coconut::RedisController>(controller); 
				boost::shared_ptr<coconut::RedisResponse> res = d->getAndDeleteResponseOfTicket(ticket);
				LOG_INFO( "REDIS RESULT : %s\n", res->result()->str.c_str());

				// redis close test
				d->redisRequest()->close();
				d->redisRequest()->connect();

				socket()->close();
			} catch (coconut::Exception &e) {
				printf("REDIS RESULT ERROR\n");
			}
		}
#endif
	}
	
	virtual void onTimer(unsigned short id) {
		LOG_INFO("[%d] MyClientController onTimer emitted.. %d\n", id_, id);
		socket()->close();
	}
	
	virtual void onReceivedData(const void *data, int size) {
		LOG_DEBUG( "[%d] MyClientController onRead emitted.. %d\n", id_, size);
	}

	int id() {
		return id_;
	}

private:
	int id_;
	coconut::ticket_t ticket_;
#ifdef TEST_HTTP
	boost::shared_ptr<MyHttpController> httpController_;
#endif
};


//==========================================================
class MyFDController : public coconut::FileDescriptorController {

public:
	MyFDController() : fd_(0) { 
	}
	
	virtual void onInitialized() {
		LOG_DEBUG( "MyFDController onInitialized emitted.. %d -> %d\n", socket()->socketFD(), fd_);

		if(fd_ > 0)
			writeDescriptor(fd_);
	}
	virtual void onConnected() {
		LOG_DEBUG( "MyFDController onConnected emitted..\n");
	}
	virtual void onReceivedData(const void *data, int size) {
		LOG_DEBUG( "MyFDController onRead emitted.. fd %d, size = %d\n", socket()->socketFD(), size);
	}
	
	virtual void onDescriptorReceived(int fd) {
		LOG_DEBUG( "MyFDController onDescriptorReceived emitted.. %d\n", fd);

		if(fd <= 0)
			return;

		while(true) {
			char buff[1024] = {0, };
			if(read(fd, buff, sizeof(buff)) > 0) {
				LOG_DEBUG( "%s", buff);
				continue;
			}
			break;
		}

		socket()->close();
	}

	void setTestFd(int fd) {
		fd_ = fd;
	}

private:
	int fd_;
};


//==========================================================
class MyUnixServerController : public coconut::ServerController {
public:
	virtual void onInitialized() {
		fd_ = open("test.txt", O_RDONLY);

		LOG_DEBUG( "MyUnixServerController onInitialized file descriptor = %d\n", fd_);
	}

	virtual boost::shared_ptr<coconut::ClientController> onAccept(boost::shared_ptr<coconut::TcpSocket> socket) {

		boost::shared_ptr<MyFDController> newController(new MyFDController); 
		newController->setTestFd(fd_);
		return newController;
	}

private:
	int fd_;
};


//==========================================================
class MyTcpServerController : public coconut::ServerController {
public:
	virtual void onInitialized() {
		//setTimer(1024, 1000);
	}

	virtual boost::shared_ptr<coconut::ClientController> onAccept(boost::shared_ptr<coconut::TcpSocket> socket) {
		boost::shared_ptr<MyClientController> newController(new MyClientController(-1)); 
		clients_.insert(newController);
		newController->eventGotProtocol()->registerObserver(this);
		return newController;
	}

	virtual void onControllerEvent_GotProtocol(
						boost::shared_ptr<coconut::BaseController> controller, 
						boost::shared_ptr<coconut::protocol::BaseProtocol> prot) {
		boost::shared_ptr<LineProtocol> d = boost::static_pointer_cast<LineProtocol>(prot); 

		LOG_DEBUG( "MyTcpServerController onControllerEvent_GotProtocol emitted.. line = %s", d->linePtr());
	}
	
	virtual void onControllerEvent_ClosedConnection(
						boost::shared_ptr<coconut::BaseController> controller, 
						int error) {
		LOG_DEBUG( "MyTcpServerController onControllerEvent_ClosedConnection emitted.. error : %d\n", error);

		boost::shared_ptr<MyClientController> d = boost::static_pointer_cast<MyClientController>(controller); 
		LOG_INFO("MyTcpServerController client closed id %d\n", d->id());

		clientset_t::iterator it = clients_.find(d);
		if(it != clients_.end()) {
			clients_.erase(it);
		}
	}

	virtual void onTimer(unsigned short id) {
		LOG_DEBUG( "MyTcpServerController onTimer emitted.. %d\n", id);
	}

private:
	typedef std::set<boost::shared_ptr<MyClientController> > clientset_t;
	clientset_t clients_;
};


//==========================================================
int main(int argc, char **argv) {
	LOG_INFO( "Entering application : %d", argc);

	coconut::IOServiceContainer ioServiceContainer(20);
//	coconut::IOServiceContainer ioServiceContainer;
	ioServiceContainer.initialize();

	try {
		std::vector<boost::shared_ptr<coconut::ClientController> > clients;

#ifdef TEST_TCP
		// tcp server test
		boost::shared_ptr<MyTcpServerController> tcpServerController(new MyTcpServerController);
		coconut::NetworkHelper::listenTcp(&ioServiceContainer, BIND_PORT, tcpServerController);

#ifdef TEST_TCP_CLIENT
		LOG_DEBUG( "=============== TCP CLIENT START =============\n");
		for(int i = 0; i < TEST_TCP_CLIENT_COUNT; i++) {
			// tcp client test
			boost::shared_ptr<MyClientController> controller(new MyClientController(i));
			//ioServiceContainer->connectTcp("was.gp.hangame.co", 8081, controller);
			coconut::NetworkHelper::connectTcp(&ioServiceContainer, "localhost", BIND_PORT, controller);

			clients.push_back(controller);
		}
		LOG_DEBUG( "=============== TCP CLIENT END ===============\n");
#endif
#endif

#ifdef TEST_UDP
		// udp server & client test
		boost::shared_ptr<MyUdpClientController> udpController(new MyUdpClientController);
		coconut::NetworkHelper::bindUdp(&ioServiceContainer, UDP_PORT, udpController);

#ifdef TEST_UDP_CLIENT
		LOG_DEBUG( "=============== UDP CLIENT START =============\n");
		for(int i = 0; i < 1; i++) {
			// tcp client test
			boost::shared_ptr<MyUdpClientController> controller(new MyUdpClientController);
			coconut::NetworkHelper::bindUdp(&ioServiceContainer, 0, controller);
			controller->udpSocket()->writeTo("TEST", 4, "localhost", 8081);

			clients.push_back(controller);
		}
		LOG_DEBUG( "=============== UDP CLIENT END ===============\n");
#endif
#endif

#ifdef TEST_UDS
		// unix server test
		boost::shared_ptr<MyUnixServerController> unixServerController(new MyUnixServerController);
		coconut::NetworkHelper::listenUnix(&ioServiceContainer, "server.sock", unixServerController);

		// unix client test
//		boost::shared_ptr<MyClientController> unixClientController(new MyClientController);
		boost::shared_ptr<MyFDController> unixClientController(new MyFDController);
		coconut::NetworkHelper::connectUnix(&ioServiceContainer, "server.sock", unixClientController);
#endif

#ifdef TEST_HTTP
		// http request test
		srand(time(NULL));
		coconut::HttpParameter params;
		params.addParameter("userid", "anycall300@hangame.com");
		params.addParameter("targetid", "anycall300@hangame.com");
		params.addParameter("sizetype", "1");
		params.addParameter("action", "down");
//		params.addParameter("action", "up");
		params.addParameter("imgcrc", rand() % 1000);
		params.addFile("profileImage", "testfilemedium.txt");
		//params.addFile("profileImage", "/home/gtalk/src/coconut/sample/test.png");

		std::string uri = "http://119.205.238.162:8081/test.php?arg=0&arg2=2&userid='tester@hangame.com'";
		//std::string uri = "http://119.205.238.162:8081/nphone/OpenTalkProfile.php?arg=0&arg2=2&userid='tester@hangame.com'";
		//std::string uri = "http://119.205.238.162:8081/nphone/MyProfileImage.php";
		//std::string uri = "http://was.gp.hangame.com:8081/nphone/OpenTalkProfile.php?arg=0&arg2=2&userid='tester@hangame.com'";
		boost::shared_ptr<MyHttpController> httpController(new MyHttpController);
		coconut::NetworkHelper::httpClient(&ioServiceContainer, coconut::HTTP_POST, uri.c_str(), 20, &params, httpController);
		//coconut::NetworkHelper::httpClient(&ioServiceContainer, coconut::HTTP_GET, uri.c_str(), 20, &params, httpController);
		LOG_DEBUG( "HTTP REQUEST ASYNC!!!\n");
#endif
		// event loop start!
		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		LOG_DEBUG( "Exception emitted : %s\n", e.what());
	}

	// exit..
	LOG_INFO( "Leaving application");
	return 0;
}

