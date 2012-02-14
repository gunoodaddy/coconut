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
#include "UdpSocket.h"
#include "IOService.h"
#include "Exception.h"
#include "IPv4Address.h"
#include "InternalLogger.h"
#include "UdpSocketImpl.h"
#include "BaseIOSystemFactory.h"

namespace coconut {

UdpSocket::UdpSocket() : BaseSocket(UDP) {
	impl_ = BaseIOSystemFactory::instance()->createUdpSocketImpl();
}

UdpSocket::UdpSocket(boost::shared_ptr<IOService> ioService, int port) : BaseSocket(UDP) {
	ioService_ = ioService;

	impl_ = BaseIOSystemFactory::instance()->createUdpSocketImpl();
	impl_->initialize(this, port);
}

UdpSocket::~UdpSocket() {
}

coconut_socket_t UdpSocket::socketFD() {
	return impl_->socketFD();
}

void UdpSocket::initialize(boost::shared_ptr<IOService> ioService, int port) {
	ioService_ = ioService;
	impl_->initialize(this, port);
}

void UdpSocket::connect() {
	impl_->connect();
}

void UdpSocket::bind() {
	impl_->bind();
}

void UdpSocket::close() {
	impl_->close();
}

void UdpSocket::checkResponseSocket(int res) {
	impl_->checkResponseSocket(res);
}

int UdpSocket::writeTo(const void *data, size_t size, const struct sockaddr_in *sin) {
	return impl_->writeTo(data, size, sin);
}

int UdpSocket::writeTo(const void *data, size_t size, const char *host, int port) {
	return impl_->writeTo(data, size, host, port);
}

int UdpSocket::write(const void *data, size_t size) {
	return impl_->write(data, size);
}

const BaseAddress * UdpSocket::peerAddress() {
	return impl_->peerAddress();
}

const BaseAddress * UdpSocket::sockAddress() {
	return impl_->sockAddress();
}

}
