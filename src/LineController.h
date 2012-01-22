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

#include "ClientController.h"
#include "BaseProtocol.h"
#include "LineProtocol.h"

namespace coconut {

class COCONUT_API LineController : public ClientController {
private:
	class LineProtocolFactory : public protocol::BaseProtocolFactory {
	public:
		boost::shared_ptr<protocol::BaseProtocol> makeProtocol() {
			boost::shared_ptr<protocol::LineProtocol> line(new protocol::LineProtocol);
			return line;
		}
	};
	void _onPreInitialized() {
		boost::shared_ptr<LineProtocolFactory> factory(new LineProtocolFactory);
		setProtocolFactory(factory);
		
		BaseController::_onPreInitialized();
	}

	void onReceivedProtocol(boost::shared_ptr<protocol::BaseProtocol> protocol);

public:
	void writeLine(const std::string &line);

protected:
	// LineController callback event
	virtual void onLineReceived(const char *line) = 0;

private:
	std::string buffer_;
};

} // end of namespace coconut
