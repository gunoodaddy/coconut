#include "Coconut.h"
#include <fcntl.h>
#include <log4cxx/logger.h> 
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include "NetworkHelper.h"
#include "LineController.h"
#include "FileDescriptorController.h"
#include "FrameProtocol.h"
#include "IOServiceContainer.h"
#include "log4cxxutil.h"

using namespace log4cxx;
using namespace coconut::protocol;
LoggerPtr logger(Logger::getLogger("MyApp"));

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
		MY_LOG4CXX_INFO(logger,"!! REDIS RESPONSE : Ticket %d => %s\n", response->ticket(), response->result()->str.c_str());
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
		MY_LOG4CXX_DEBUG(logger, "onReadFrom emitted.. %d from %s:%d\n", size, inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));
	}
	*/

	virtual void onLineReceived(const char *line) {
		MY_LOG4CXX_DEBUG(logger, "MyUdpClientController LINE RECEIVED : [%s]\n", line);
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
class MyHttpController : public coconut::HttpRequestController
{
	virtual void onReceivedChucked(int receivedsize) { 
		MY_LOG4CXX_DEBUG(logger, "MyHttpController received size : %d byte\n", receivedsize);
	}

	virtual void onError(coconut::HttpRequest::ErrorCode errorcode) {
		MY_LOG4CXX_DEBUG(logger, "MyHttpController onError : %d\n", errorcode);
	}

	virtual void onResponse(int rescode) {
		MY_LOG4CXX_INFO(logger,"MyHttpController onResponse [this = %p]\n", this);
		MY_LOG4CXX_INFO(logger,"Header [%s]\n", httpRequest()->findHeader("profimg-name-base64").c_str());

		int fd = ::open("result", O_CREAT | O_TRUNC | O_WRONLY, 00644);
		int len = write(fd, httpRequest()->responseBody(), httpRequest()->responseBodySize());
		MY_LOG4CXX_INFO(logger,"HTTP RESULT SAVE FILE : %d bytes\n", len);
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
		MY_LOG4CXX_DEBUG(logger, "[%d] MyClientController LINE RECEIVED : [%s]\n", id_, line);
//		socket()->write(line, strlen(line));
//		socket()->write("\r\n", 2);
	}

	virtual void onError(int error, const char *strerror) {
		MY_LOG4CXX_DEBUG(logger, "[%d] MyClientController onError emitted.. %d %s, thread = %p, socket = %p\n", 
				id_, error, strerror, (void*)pthread_self(), socket().get());
	}

	virtual void onClosed() { 
		MY_LOG4CXX_DEBUG(logger, "[%d] MyClientController onClose emitted..\n", id_);
	}

	virtual void onConnected() {
		MY_LOG4CXX_DEBUG(logger, "[%d] MyClientController onConnected emitted.. thread = %p\n", id_, (void *)pthread_self());
		socket()->write((const void *)"HELLO\r\n", 7);

#ifdef TEST_REDIS
		ticket_ = MyRedisController::instanceOfSingleton(ioServiceContainer())->redisRequest()->get("keh");
		MyRedisController::instanceOfSingleton(ioServiceContainer())->eventGotResponse()->registerObserver(ticket_, this);
#endif

#ifdef TEST_HTTP
		std::string uri = "http://119.205.238.162:8081/test.php?arg=0&arg2=2&userid='tester@hangame.com'";
		boost::shared_ptr<MyHttpController> httpController(new MyHttpController);
		coconut::NetworkHelper::httpRequest(ioServiceContainer(), coconut::HTTP_POST, uri.c_str(), 20, NULL, httpController);

		httpController_ = httpController;
		httpController->eventGotResponse()->registerObserver(0, this);
#endif
	}

	virtual void onControllerEvent_GotResponse(
						boost::shared_ptr<coconut::BaseController> controller, 
						int ticket) {
		MY_LOG4CXX_INFO(logger,"[%d] MyClientController onControllerEvent_GotResponse emitted.. ticket %d\n", id_, ticket);

#ifdef TEST_HTTP
		if(httpController_.get() == controller.get()) {
			boost::shared_ptr<MyHttpController> d = boost::static_pointer_cast<MyHttpController>(controller); 
			MY_LOG4CXX_DEBUG(logger, "d.responseBody() ==>[this = %p]\n%s\n", controller.get(), (char *)d->httpRequest()->responseBody());
		}
			// for close test
			//setTimer(1024, 1000, false);
#endif

#ifdef TEST_REDIS
		if(MyRedisController::instanceOfSingleton().get() == controller.get()) {
			try {
				boost::shared_ptr<coconut::RedisController> d = boost::static_pointer_cast<coconut::RedisController>(controller); 
				boost::shared_ptr<coconut::RedisResponse> res = d->getAndDeleteResponseOfTicket(ticket);
				MY_LOG4CXX_INFO(logger, "REDIS RESULT : %s\n", res->result()->str.c_str());

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
		MY_LOG4CXX_INFO(logger,"[%d] MyClientController onTimer emitted.. %d\n", id_, id);
		socket()->close();
	}
	
	virtual void onReceivedData(const void *data, int size) {
		MY_LOG4CXX_DEBUG(logger, "[%d] MyClientController onRead emitted.. %d\n", id_, size);
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
		MY_LOG4CXX_DEBUG(logger, "MyFDController onInitialized emitted.. %d -> %d\n", socket()->socketFD(), fd_);

		if(fd_ > 0)
			writeDescriptor(fd_);
	}
	virtual void onConnected() {
		MY_LOG4CXX_DEBUG(logger, "MyFDController onConnected emitted..\n");
	}
	virtual void onReceivedData(const void *data, int size) {
		MY_LOG4CXX_DEBUG(logger, "MyFDController onRead emitted.. fd %d, size = %d\n", socket()->socketFD(), size);
	}
	
	virtual void onDescriptorReceived(int fd) {
		MY_LOG4CXX_DEBUG(logger, "MyFDController onDescriptorReceived emitted.. %d\n", fd);

		if(fd <= 0)
			return;

		while(true) {
			char buff[1024] = {0, };
			if(read(fd, buff, sizeof(buff)) > 0) {
				MY_LOG4CXX_DEBUG(logger, "%s", buff);
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

		MY_LOG4CXX_DEBUG(logger, "MyUnixServerController onInitialized file descriptor = %d\n", fd_);
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

		MY_LOG4CXX_DEBUG(logger, "MyTcpServerController onControllerEvent_GotProtocol emitted.. line = %s", d->linePtr());
	}
	
	virtual void onControllerEvent_ClosedConnection(
						boost::shared_ptr<coconut::BaseController> controller, 
						int error) {
		MY_LOG4CXX_DEBUG(logger, "MyTcpServerController onControllerEvent_ClosedConnection emitted.. error : %d\n", error);

		boost::shared_ptr<MyClientController> d = boost::static_pointer_cast<MyClientController>(controller); 
		MY_LOG4CXX_INFO(logger,"MyTcpServerController client closed id %d\n", d->id());

		clientset_t::iterator it = clients_.find(d);
		if(it != clients_.end()) {
			clients_.erase(it);
		}
	}

	virtual void onTimer(unsigned short id) {
		MY_LOG4CXX_DEBUG(logger, "MyTcpServerController onTimer emitted.. %d\n", id);
	}

private:
	typedef std::set<boost::shared_ptr<MyClientController> > clientset_t;
	clientset_t clients_;
};


//==========================================================
int main(int argc, char **argv) {
	if(argc > 1)
		PropertyConfigurator::configure(argv[1]);
	else
		PropertyConfigurator::configure("log4cxx_tester.properties");

	MY_LOG4CXX_INFO(logger, "Entering application : %d", argc);

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
		MY_LOG4CXX_DEBUG(logger, "=============== TCP CLIENT START =============\n");
		for(int i = 0; i < TEST_TCP_CLIENT_COUNT; i++) {
			// tcp client test
			boost::shared_ptr<MyClientController> controller(new MyClientController(i));
			//ioServiceContainer->connectTcp("was.gp.hangame.co", 8081, controller);
			coconut::NetworkHelper::connectTcp(&ioServiceContainer, "localhost", BIND_PORT, controller);

			clients.push_back(controller);
		}
		MY_LOG4CXX_DEBUG(logger, "=============== TCP CLIENT END ===============\n");
#endif
#endif

#ifdef TEST_UDP
		// udp server & client test
		boost::shared_ptr<MyUdpClientController> udpController(new MyUdpClientController);
		coconut::NetworkHelper::bindUdp(&ioServiceContainer, UDP_PORT, udpController);

#ifdef TEST_UDP_CLIENT
		MY_LOG4CXX_DEBUG(logger, "=============== UDP CLIENT START =============\n");
		for(int i = 0; i < 1; i++) {
			// tcp client test
			boost::shared_ptr<MyUdpClientController> controller(new MyUdpClientController);
			coconut::NetworkHelper::bindUdp(&ioServiceContainer, 0, controller);
			controller->udpSocket()->writeTo("TEST", 4, "localhost", 8081);

			clients.push_back(controller);
		}
		MY_LOG4CXX_DEBUG(logger, "=============== UDP CLIENT END ===============\n");
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
		coconut::NetworkHelper::httpRequest(&ioServiceContainer, coconut::HTTP_POST, uri.c_str(), 20, &params, httpController);
		//coconut::NetworkHelper::httpRequest(&ioServiceContainer, coconut::HTTP_GET, uri.c_str(), 20, &params, httpController);
		MY_LOG4CXX_DEBUG(logger, "HTTP REQUEST ASYNC!!!\n");
#endif
		// event loop start!
		ioServiceContainer.run();
	} catch(coconut::Exception &e) {
		MY_LOG4CXX_DEBUG(logger, "Exception emitted : %s\n", e.what());
	}

	// exit..
	MY_LOG4CXX_INFO(logger, "Leaving application");
	return 0;
}

