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
#include "Exception.h"
#include "InternalLogger.h"
#include "config.h"
#include "BaseIOServiceContainer.h"
#include "ThreadUtil.h"
#include "IOServiceImpl.h"
#include "IOSystemFactory.h"
#include "LibeventSystemFactory.h"

namespace coconut {

#if defined(WIN32)	
static bool gStartUpWinSock = false;
#endif

extern void activateMultithreadMode();

class UniqueIntKeyMaker {
public:
	UniqueIntKeyMaker() { }

	int makeKey() {
		ScopedMutexLock(lock_);
		int i = 0;
		while(true) {
			std::list<int>::iterator it = std::find(keys_.begin(), keys_.end(), i);
			if(it == keys_.end())
				return i;
			i++;
		}
	}

private:
	std::list<int> keys_;
	Mutex lock_;
};
UniqueIntKeyMaker gKeyMaker_;
//---------------------------------------------------------------------------------------------------

class DefaultIOServiceContainer: public BaseIOServiceContainer {
public:
	DefaultIOServiceContainer(boost::shared_ptr<IOService> ioService) : ioService_(ioService_) { }
	~DefaultIOServiceContainer() { }

public:
	static DefaultIOServiceContainer * instance(boost::shared_ptr<IOService> ioService) {
		lock_.lock();
		if(!selfInstance_) {
			selfInstance_ = boost::shared_ptr<DefaultIOServiceContainer>(new DefaultIOServiceContainer(ioService));
			selfInstance_->initialize();
		} else {
			selfInstance_->setIOService(ioService);
		}
		lock_.unlock();
		return selfInstance_.get();
	}

	virtual size_t ioServiceCount() { return 0; }
	virtual boost::shared_ptr<IOService> ioServiceByRoundRobin() { return ioService_; }
	virtual boost::shared_ptr<IOService> ioServiceByIndex(size_t index) { return ioService_; }
	
	void initialize() {
		IOSystemFactory::setInstance(boost::shared_ptr<IOSystemFactory>(new LibeventSystemFactory));
	}
	void run() {
		assert(false && "never call this function : run()");
	}
	void stop() {
		ioService_->stop();
	}

private:
	void setIOService(boost::shared_ptr<IOService> ioService) { ioService_ = ioService; }

private:
	boost::shared_ptr<IOService> ioService_;
	static Mutex lock_;
	static boost::shared_ptr<DefaultIOServiceContainer> selfInstance_;
};
Mutex DefaultIOServiceContainer::lock_;
boost::shared_ptr<DefaultIOServiceContainer> DefaultIOServiceContainer::selfInstance_;
boost::shared_ptr<DefaultIOServiceContainer> globalDefaultIOServiceContainer_;

//---------------------------------------------------------------------------------------------------

IOService::IOService() {
	int key = gKeyMaker_.makeKey();

	BaseIOServiceContainer *ioServiceContainer = DefaultIOServiceContainer::instance(shared_from_this());
	impl_ = IOSystemFactory::instance()->createIOServiceImpl(key, ioServiceContainer, false);
}

IOService::IOService(BaseIOServiceContainer *ioServiceContainer, bool threadMode) {
	if(NULL == ioServiceContainer)
		ioServiceContainer = DefaultIOServiceContainer::instance(shared_from_this());
		
	int key = gKeyMaker_.makeKey();
	impl_ = IOSystemFactory::instance()->createIOServiceImpl(key, ioServiceContainer, threadMode);
}

IOService::~IOService() {
}

boost::thread::id IOService::threadHandle() {
	return impl_->threadHandle();
}

boost::thread::native_handle_type IOService::nativeThreadHandle() {
	return impl_->nativeThreadHandle();
}

bool IOService::isCalledInMountedThread() {
	return impl_->isCalledInMountedThread();
}

void IOService::deferredCall(deferedMethod_t func) {
	return impl_->deferredCall(func);
}

bool IOService::isStopped() {
	return impl_->isStopped();
}

BaseIOServiceContainer *IOService::ioServiceContainer() {
	return impl_->ioServiceContainer();
}

coconut_io_handle_t IOService::coreHandle() {
	return impl_->coreHandle();
}

Mutex & IOService::mutex() {
	return impl_->mutex();
}

int IOService::id() {
	return impl_->id();
}

void IOService::initialize() {
	impl_->initialize();
}

void IOService::run() {
	impl_->run();
}

void IOService::stop() {
	impl_->stop();
}

void IOService::runWithoutJoinThread() {
	impl_->_startEventLoopInThread();
}

void IOService::joinThread() {
	impl_->_joinThread();
}

#if defined(WIN32)
void IOService::turnOnIOCP(size_t cpuCnt) {
	impl_->turnOnIOCP(cpuCnt);
}
#endif

}
