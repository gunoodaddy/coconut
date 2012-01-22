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
