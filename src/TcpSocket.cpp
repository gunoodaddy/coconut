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

#include "CoconutLib.h"
#include "IOService.h"
#include "IPv4Address.h"
#include "TcpSocket.h"
#include "BaseSocket.h"
#include "InternalLogger.h"
#include "Exception.h"
#include <string>
#include "TcpSocketImpl.h"
#include "BaseIOSystemFactory.h"

namespace coconut {
	
TcpSocket::TcpSocket() : BaseSocket(TCP) {
	impl_ = BaseIOSystemFactory::instance()->createTcpSocketImpl();
	impl_->initialize(this);
}

TcpSocket::~TcpSocket() {
}

void TcpSocket::initialize(boost::shared_ptr<IOService> ioService) {
	ioService_ = ioService;
	impl_->initialize(this);
}

coconut_socket_t TcpSocket::socketFD() {
	return impl_->socketFD();
}

void TcpSocket::attachSocketHandle(coconut_socket_t fd, bool doInstallFlag) {
	impl_->attachSocketHandle(fd, doInstallFlag);
}

void TcpSocket::connect(const char *host, int port, int timeout) {
	impl_->connect(host, port, timeout);
}

void TcpSocket::connectUnix(const char *path, int timeout) {
	impl_->connectUnix(path, timeout);
}

void TcpSocket::connect() {
	impl_->connect();
}

void TcpSocket::install() {
	impl_->install();
}

void TcpSocket::checkResponseSocket(int res) {
	impl_->checkResponseSocket(res);
}

int TcpSocket::read(std::string &data, size_t size) {
	return impl_->read(data, size);
}

int TcpSocket::read(void *data, size_t size) {
	return impl_->read(data, size);
}

int TcpSocket::write(const void *data, size_t size) {
	return impl_->write(data, size);
}

int TcpSocket::writeString(const std::string &data) {
	return write((const void *)data.c_str(), data.size());
}

void TcpSocket::close() {
	return impl_->close();
}

void TcpSocket::closeAfterAllSent() {
	return impl_->closeAfterAllSent();
}

const BaseAddress * TcpSocket::peerAddress() {
	return impl_->peerAddress();
}

const BaseAddress * TcpSocket::sockAddress() {
	return impl_->sockAddress();
}

}
