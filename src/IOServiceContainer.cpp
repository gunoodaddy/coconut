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

#include "Coconut.h"
#include "IOService.h"
#include "IOServiceContainer.h"
#include "Exception.h"
#include "InternalLogger.h"

namespace coconut {

extern bool _activateMultithreadMode_on;
extern void activateMultithreadMode();

IOServiceContainer::~IOServiceContainer() {
	_LOG_TRACE("~IOServiceContainer() : %p", this);
}

#if defined(WIN32)
void IOServiceContainer::turnOnIOCP(size_t cpuCnt) {
	
	if(ioservices_.size() > 0) {
		throw Exception("turnOnIOCP must be called before initialize()");
	} else if(threadCount_ > 0) {
		throw Exception("iocp is turned on, but your thread count setting is not 0");
	}

	cpuCnt_ = cpuCnt;
	if(cpuCnt_ == 0)
		cpuCnt_ = 1;

	iocpEnabled_ = true;
}
#endif

void IOServiceContainer::initialize() {
	if(threadCount_ < 0) {
		new Exception("thread cound is invalid.");
	}

#if defined(WIN32) 
	if(iocpEnabled_) {
		boost::shared_ptr<IOService> ioservice(new IOService(this, false));
		ioservice->turnOnIOCP(cpuCnt_);	// need cpu count..
		ioservice->initialize();
		ioservices_.push_back(ioservice);
	}
	else
#endif
	{
		if(threadCount_ > 0) {
			activateMultithreadMode();
			for(int i = 0; i < threadCount_; i++) {
				boost::shared_ptr<IOService> ioservice(new IOService(this, true));
				ioservice->initialize();
				ioservices_.push_back(ioservice);
			}
		} else {
			boost::shared_ptr<IOService> ioservice(new IOService(this, false));
			ioservice->initialize();
			ioservices_.push_back(ioservice);
		}
	}
}

boost::shared_ptr<IOService> IOServiceContainer::ioServiceByRoundRobin() {
	static volatile boost::uint32_t s_index = 0;
	int id = atomicIncreaseInt32(&s_index) % ioservices_.size();
	_LOG_DEBUG("################## EVENT BASE %02d DEPLOYMENT ##################", id);
	return ioservices_[id];
}


boost::shared_ptr<IOService> IOServiceContainer::ioServiceByIndex(size_t index) {
	if(index >= ioservices_.size())
		throw IllegalArgumentException();

	return ioservices_[index];
}


void IOServiceContainer::run() {
	
	if(ioservices_.size() <= 0) {
		throw Exception("initialize() method must be called before run()");
	}

	bool threadMode = threadCount_ > 0;

#if defined(WIN32) 
	if(iocpEnabled_) 
		threadMode = false; // on libevent iocp, struct event_base's count is one. 
#endif

	if(threadMode) {
		for(size_t i = 0; i < ioservices_.size(); i++) {
			ioservices_[i]->runWithoutJoinThread();
		}

		for(size_t i = 0; i < ioservices_.size(); i++) {
			ioservices_[i]->joinThread();
		}
	} else {
		if(ioservices_.size() != 1) {
			throw Exception("unexpacted case! thread mode is false, but io service's count is not 1..");
		}
		ioservices_[0]->run();
	}

	_LOG_INFO("All IOService finished..");
}

void IOServiceContainer::stop() {
	for(size_t i = 0; i < ioservices_.size(); i++) {
		ioservices_[i]->stop();
	}
}

}
