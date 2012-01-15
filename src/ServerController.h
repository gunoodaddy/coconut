#pragma once

#include <set>
#include "BaseController.h"
#include "ConnectionListener.h"
#include "TcpSocket.h"

namespace coconut {

class IOService;
class ClientController;

class COCONUT_API ServerController : public BaseController
                                   , private ConnectionListener::EventHandler {
private:
	static const int TIMERID_DELAYED_REMOVE = (1|INTERNAL_TIMER_BIT);

public:
	ServerController() : ioServiceContainer_(NULL) {
	}

	virtual ~ServerController();
	
public:
	// strategy pattern
	void setConnectionListener(boost::shared_ptr<ConnectionListener> connListener) { 
		connListener_ = connListener;
		connListener->setEventHandler(this);

		_onPreInitialized();
	}

	boost::shared_ptr<ConnectionListener> connListener() {
		return connListener_;
	}

	boost::shared_ptr<IOService> ioService();
	BaseIOServiceContainer *ioServiceContainer();
	
private:
	void processDelayedRemoveClientFromSet();

private:
	void onConnectionListener_Accept(coconut_socket_t newSocket);
	void onConnectionListener_Error(int error);
	void onTimer_Timer(int id);

	void _onPreControllerEvent_OccuredError(
					boost::shared_ptr<coconut::BaseController> controller, 
					int error);

	void _onPreControllerEvent_ClosedConnection(
					boost::shared_ptr<coconut::BaseController> controller, 
					int error);
protected:
	// ServerController callback event
	virtual boost::shared_ptr<ClientController> onAccept(boost::shared_ptr<TcpSocket> socket) = 0;
	virtual void onError(int error) { }

private:
	BaseIOServiceContainer *ioServiceContainer_;
	boost::shared_ptr<ConnectionListener> connListener_;

	typedef std::set<boost::shared_ptr<BaseController> > clientset_t;
	clientset_t clients_;
	clientset_t delay_remove_clients_set_;
	Mutex lockClients_;
};

} // end of namespace coconut
