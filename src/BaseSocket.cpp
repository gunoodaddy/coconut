#include "Coconut.h"
#include "BaseSocket.h"
#include "IOService.h"
#include "ThreadUtil.h"

namespace coconut {

void BaseSocket::fire_onSocket_Initialized() {
	lockHandler_.lock();
	if(handler_ && ioService_->isStopped() == false)
		handler_->onSocket_Initialized();
	lockHandler_.unlock();
}

void BaseSocket::fire_onSocket_Connected() {
	lockHandler_.lock();
	if(handler_ && ioService_->isStopped() == false)
		handler_->onSocket_Connected();
	lockHandler_.unlock();
}

void BaseSocket::fire_onSocket_Error(int error, const char *strerror) {
	lockHandler_.lock();
	if(handler_ && ioService_->isStopped() == false)
		handler_->onSocket_Error(error, strerror);
	lockHandler_.unlock();	
}

void BaseSocket::fire_onSocket_ReadEvent(int fd) {
	lockHandler_.lock();
	if(handler_ && ioService_->isStopped() == false)
		handler_->onSocket_ReadEvent(fd);
	lockHandler_.unlock();	
}

void BaseSocket::fire_onSocket_ReadFrom(const void *data, int size, struct sockaddr_in * sin) {
	lockHandler_.lock();
	if(handler_ && ioService_->isStopped() == false)
		handler_->onSocket_ReadFrom(data, size, sin);
	lockHandler_.unlock();
}

void BaseSocket::fire_onSocket_Close() {
	lockHandler_.lock();
	if(handler_ && ioService_->isStopped() == false)
		handler_->onSocket_Close();
	lockHandler_.unlock();
}

}
