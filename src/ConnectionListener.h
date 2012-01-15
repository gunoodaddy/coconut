#pragma once

namespace coconut {

class IOService;
class ConnectionListenerImpl;

class COCONUT_API ConnectionListener {
public:
	ConnectionListener(boost::shared_ptr<IOService> ioService, int port);
	ConnectionListener(boost::shared_ptr<IOService> ioService, const char* path);
	~ConnectionListener(void);

	class COCONUT_API EventHandler {
	public:
		virtual ~EventHandler() { }

		virtual void onConnectionListener_Accept(coconut_socket_t newSocket) = 0;
		virtual void onConnectionListener_Error(int error) { }
	};

public:
	void setEventHandler(EventHandler *handler) {
		handler_ = handler;
	}

	EventHandler* eventHandler() {
		return handler_;
	}

	boost::shared_ptr<IOService> ioService() {
		return ioService_;
	}

	void listen();
	const char * listeningPath();

private:
	ConnectionListenerImpl *impl_;
	boost::shared_ptr<IOService> ioService_;	
	EventHandler *handler_;
};

}
