#pragma once

#include "BaseSocket.h"
#include "BaseProtocol.h"
#include "TcpSocket.h"
#include "UdpSocket.h"
#include "BaseController.h"

namespace coconut {

class IOService;

class COCONUT_API ClientController : public BaseController
                       , public BaseSocket::EventHandler
{
public:
	static const int TIMERID_RECONNECT = (1|INTERNAL_TIMER_BIT);
public:
	ClientController() : ioServiceContainer_(NULL), reconnectable_(false), initFlag_(false), retryConnectCnt_(0) {
	}

	virtual ~ClientController();

public:
	// strategy pattern
	void setSocket(boost::shared_ptr<BaseSocket> socket, bool callInit = true) { 
		socket_ = socket;

		if(callInit) {
			fireOnInitialized();
		}
	}

	void setProtocolFactory(boost::shared_ptr<protocol::BaseProtocolFactory> factory) {
		protocolFactory_ = factory;
	}

	boost::shared_ptr<BaseSocket> socket() {
		return socket_;
	}

	boost::shared_ptr<TcpSocket> tcpSocket() {
		assert(socket_->type() == TCP && "invalid socket type : TCP");

		return boost::static_pointer_cast<TcpSocket>(socket_);
	}

	boost::shared_ptr<UdpSocket> udpSocket() {
		assert(socket_->type() == UDP && "invalid socket type : UDP");

		return boost::static_pointer_cast<UdpSocket>(socket_);
	}

	boost::shared_ptr<IOService> ioService();
	BaseIOServiceContainer* ioServiceContainer();

	void setReconnectable(bool enable);

private:
	void processReconnect();
	void fireOnInitialized() {
		if(false == initFlag_) {
			initFlag_ = true;
			_onPreInitialized();
		}
	}

private:
	void onSocket_Connected();
	void onSocket_Error(int error, const char*strerror);
	void onSocket_Close();
	void onSocket_ReadFrom(const void *data, int size, struct sockaddr_in * sin);
	void onSocket_ReadEvent(int fd);
	void onTimer_Timer(int id);

protected:
	// ClientController callback event
	virtual void onReceivedProtocol(boost::shared_ptr<protocol::BaseProtocol> protocol) { }
	virtual void onReceivedData(const void *data, int size) { }
	virtual void onReceivedDatagram(const void *data, int size, const struct sockaddr_in *sin) { }
	virtual void onConnected() { }
	virtual void onClosed() { }
	virtual void onError(int error, const char *strerror) { }

	
protected:
	friend class ServerController;
	BaseIOServiceContainer *ioServiceContainer_;
	boost::shared_ptr<BaseSocket> socket_;
	boost::shared_ptr<protocol::BaseProtocolFactory> protocolFactory_;
	boost::shared_ptr<protocol::BaseProtocol> protocol_;
	bool reconnectable_;
	bool initFlag_;
	int retryConnectCnt_;
};

class COCONUT_API BinaryController : public ClientController {	// TODO : divide this class COCONUT_API to another file..
private:
};


} // end of namespace coconut
