#include "Coconut.h"
#include "IOService.h"
#include "IOServiceContainer.h"
#include "Exception.h"
#include "Logger.h"

namespace coconut {

extern bool _activateMultithreadMode_on;
extern void activateMultithreadMode();

//##################################################################################

IOServiceContainer::~IOServiceContainer() {
	LOG_TRACE("~IOServiceContainer() : %p", this);
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

boost::shared_ptr<IOService> IOServiceContainer::ioService() {
	static int s_index = 0;
	int id = s_index++ % ioservices_.size();
	LOG_DEBUG("################## EVENT BASE %02d DEPLOYMENT ##################", id);
	return ioservices_[id];
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
}

void IOServiceContainer::stop() {
	for(size_t i = 0; i < ioservices_.size(); i++) {
		ioservices_[i]->stop();
	}
}

}
