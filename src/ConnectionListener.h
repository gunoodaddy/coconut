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
#include "BaseObjectAllocator.h"

namespace coconut {

class IOService;
class ConnectionListenerImpl;

class COCONUT_API ConnectionListener : public BaseObjectAllocator<ConnectionListener>
{
public:
	ConnectionListener();
	ConnectionListener(boost::shared_ptr<IOService> ioService, int port);
	ConnectionListener(boost::shared_ptr<IOService> ioService, const char* path);
	~ConnectionListener(void);

	void initialize(boost::shared_ptr<IOService> ioService, int port);
	void initialize(boost::shared_ptr<IOService> ioService, const char* path);

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
	boost::shared_ptr<ConnectionListenerImpl> impl_;
	boost::shared_ptr<IOService> ioService_;	
	EventHandler *handler_;
};

}
