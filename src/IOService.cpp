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
#include <event2/event.h>
#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/bind.hpp>
#endif

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
		if(!selfInstance_)
			selfInstance_ = boost::shared_ptr<DefaultIOServiceContainer>(new DefaultIOServiceContainer(ioService));
		else
			selfInstance_->setIOService(ioService);
		lock_.unlock();
		return selfInstance_.get();
	}

	virtual size_t ioServiceCount() { return 0; }
	virtual boost::shared_ptr<IOService> ioServiceByRoundRobin() { return ioService_; }
	virtual boost::shared_ptr<IOService> ioServiceByIndex(size_t index) { return ioService_; }
	
	void initialize() {
		assert(false && "never call this function : initialize()");
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

class IOServiceImpl {
public:
	IOServiceImpl(int id, BaseIOServiceContainer *ioServiceContainer, bool threadMode) : ioServiceContainer_(ioServiceContainer)
		, id_(id)
		, multithread_(threadMode)
		, finalizedFlag_(false)
		, loopExitFlag_(false)
		, joinedThreadFlag_(false)
#if defined(WIN32)
		, cpuCnt_(0)
		, enabledIOCP_(false)
#endif
		, base_cfg_(NULL)
		, base_(NULL)
		, priority_(NORMAL)
#ifdef __USE_PTHREAD__
		, thread_(0)
#else
		, thread_()
#endif
		, event_(NULL) { }

	~IOServiceImpl() {
		finalize();
		_LOG_TRACE("~IOServiceImpl() this = %p", this);
	}

public:
#ifdef __USE_PTHREAD__
	pthread_t threadHandle() {
		return thread_;
	}

#else
	boost::thread::id threadHandle() {
		if(!multithread_)
			return boost::this_thread::get_id();
		return thread_.get_id();
	}
#endif

#ifdef __USE_PTHREAD__
	pthread_t nativeThreadHandle() {
		return thread_;
	}
#else
	boost::thread::native_handle_type nativeThreadHandle() {
		return thread_.native_handle();
	}
#endif

	bool isCalledInMountedThread() {
		if(!multithread_) {
			return true;	// always true
		}
#ifdef __USE_PTHREAD__
		return threadHandle() == pthread_self();
#else
		return threadHandle() == boost::this_thread::get_id();
#endif
	}

	void deferredCall(IOService::deferedMethod_t func) {
		lockDeferredCaller_.lock();
		deferredCallbacks_.push_back(func);
		lockDeferredCaller_.unlock();

		// this function must be called last in this function for preventing from race-condition.
		event_active(event_, EV_READ, 1);
	}

	int id() {
		return id_;
	}

	bool isStopped() {
		return loopExitFlag_;
	}

	BaseIOServiceContainer *ioServiceContainer() {
		return ioServiceContainer_;
	}

	struct event_base * coreHandle() {
		return base_;
	}

	Mutex &mutex() {
		return lock_;
	}

	void initialize() {
#if defined(WIN32)	
		if(false == gStartUpWinSock) {
			WSADATA wsaData;
			::WSAStartup(MAKEWORD(2, 2), &wsaData);
			gStartUpWinSock = true;
		}

		if(enabledIOCP_) 
		{
			activateMultithreadMode();	
			base_cfg_ = event_config_new();	
			event_config_set_num_cpus_hint(base_cfg_, cpuCnt_);       
			event_config_set_flag(base_cfg_, EVENT_BASE_FLAG_STARTUP_IOCP);		
			base_ = event_base_new_with_config(base_cfg_);
		} 
		else
#endif
		{
			base_ = event_base_new();
		}

		event_ = event_new(base_, -1, EV_READ|EV_PERSIST, deferred_event_cb, this);
		event_add(event_, NULL);
	}

	void finalize() {
		if(finalizedFlag_)
			return;
		finalizedFlag_ = true;

		stop();
		_joinThread();

		if(base_cfg_) {
			event_config_free(base_cfg_);
			base_cfg_ = NULL;
		}

		if(event_) {
			event_free(event_);
			event_ = NULL;
		}

		if(base_) {
			// !CAUTION!
			// base_ MUST BE DELETED IN LAST LIFE CYCLE!!! (libevent principle..)
			// all struct event pointers must free before struct event_base is not freed.
			// THIS FUNCTION MUST BE CALLED IN DESTRUCTOR.
			event_base_free(base_);
			base_ = NULL;
		}
	}

	void run() {
		if(multithread_) {
			_startEventLoopInThread();
			_joinThread();
		} else {
			_startEventLoopBlock();
		}
	}

#if defined(WIN32)
	void turnOnIOCP(size_t cpuCnt) {
		cpuCnt_ = cpuCnt;
		enabledIOCP_ = true;
	}
#endif

	void stop() {
		if(false == loopExitFlag_) {
			_LOG_DEBUG("IOService stop eventloop..");
			struct timeval tv = MAKE_TIMEVAL_MSEC(10);
			event_base_loopexit(base_, &tv);
			loopExitFlag_ = true;
			//event_base_loopbreak(base_);
		}
	}

	void _startEventLoopBlock() {
		dispatchEvent();
	}

#ifdef __USE_PTHREAD__
	static int setThreadPriority(pthread_attr_t *thread_attr, ThreadPriority priority) {
		int pthread_policy = SCHED_RR;
		int min_priority = 0;
		int max_priority = 0;
		min_priority = sched_get_priority_min(pthread_policy);
		max_priority = sched_get_priority_max(pthread_policy);
		int quanta = (HIGHEST - LOWEST) + 1;
		float stepsperquanta = (max_priority - min_priority) / quanta;

		int pthreadPriority = (int)(min_priority + stepsperquanta * priority);

		struct sched_param sched_param;
		sched_param.sched_priority = pthreadPriority;

		return pthread_attr_setschedparam(thread_attr, &sched_param);
	}
#endif

	void _startEventLoopInThread() {
#ifdef __USE_PTHREAD__
		pthread_attr_t thread_attr;
		if (pthread_attr_init(&thread_attr) != 0) {
			throw ThreadException("pthread_attr_init failed");
		}

		bool detached = false;
		if(pthread_attr_setdetachstate(&thread_attr,
					detached ?
					PTHREAD_CREATE_DETACHED :
					PTHREAD_CREATE_JOINABLE) != 0) {
			throw ThreadException("pthread_attr_setdetachstate failed");
		}

		// Set thread stack size
		if (pthread_attr_setstacksize(&thread_attr, 1*1024*1024) != 0) {
			throw ThreadException("pthread_attr_setstacksize failed");
		}

		// Set thread policy
		if (pthread_attr_setschedpolicy(&thread_attr, SCHED_RR) != 0) {
			throw ThreadException("pthread_attr_setschedpolicy failed");
		}

		// Set thread priority
		if(setThreadPriority(&thread_attr, priority_) != 0) {
			throw ThreadException("pthread_attr_setschedparam failed");
		}

		if (pthread_create(&thread_, &thread_attr, threadMain, (void *)this) != 0) {
			throw ThreadException("pthread_create failed");
		}
#else
		thread_ = boost::thread(boost::bind(&IOServiceImpl::dispatchEvent, this));
#endif
	}

	int _joinThread() {
		if(multithread_ && false == joinedThreadFlag_) {
			int ret = 0;
#ifdef __USE_PTHREAD__
			assert(thread_ && "multithread mode is ON but why thread_ is NULL?");
			void* ignore;
			ret = pthread_join(thread_, &ignore);
#else
			thread_.join();
			joinedThreadFlag_ = true;
			_LOG_DEBUG("joined thread.. this = %p", this);
#endif
			return ret;
		}
		return 0;
	}

	void dispatchEvent() {
		event_base_dispatch(base_);

		_LOG_DEBUG("finished dispatch event.. this = %p", this);
		// TODO gracefully program exit logic need...
		//assert(false && "event loop exit???? why?");
	}

private:
#ifdef __USE_PTHREAD__
	static void *threadMain(void *arg) {
		IOService *SELF = (IOService *)arg;
		SELF->dispatchEvent();
		return NULL;
	}
#endif

	static void deferred_event_cb(coconut_socket_t fd, short what, void *arg) {
		IOServiceImpl *SELF = (IOServiceImpl *)arg;
		SELF->fireDeferredEvent();
	}

	void fireDeferredEvent() {
		lockDeferredCaller_.lock();
		for(size_t i = 0; i < deferredCallbacks_.size(); i++) {
			deferredCallbacks_[i]();
		}
		deferredCallbacks_.clear();
		lockDeferredCaller_.unlock();
	}

private:
	BaseIOServiceContainer *ioServiceContainer_;
	int id_;
	volatile bool multithread_;
	volatile bool finalizedFlag_;
	volatile bool loopExitFlag_;
	volatile bool joinedThreadFlag_;
#if defined(WIN32)
	size_t cpuCnt_;	// for iocp
	bool enabledIOCP_;
#endif
	struct event_config * base_cfg_;
	struct event_base * base_;
	ThreadPriority priority_;
#ifdef __USE_PTHREAD__
	pthread_t thread_;
#else
	boost::thread thread_;
#endif
	
	Mutex lock_;
	Mutex lockDeferredCaller_;
	struct event *event_;
	std::vector<IOService::deferedMethod_t> deferredCallbacks_;
};



//--------------------------------------------------------------------------------------------------

IOService::IOService() {
	int key = gKeyMaker_.makeKey();

	BaseIOServiceContainer *ioServiceContainer = DefaultIOServiceContainer::instance(shared_from_this());
	impl_ = new IOServiceImpl(key, ioServiceContainer, false);
}

IOService::IOService(BaseIOServiceContainer *ioServiceContainer, bool threadMode) {
	if(NULL == ioServiceContainer)
		ioServiceContainer = DefaultIOServiceContainer::instance(shared_from_this());
		
	int key = gKeyMaker_.makeKey();
	impl_ = new IOServiceImpl(key, ioServiceContainer, threadMode);
}

IOService::~IOService() {
	delete impl_;
}

#ifdef __USE_PTHREAD__
pthread_t IOService::threadHandle() {
	return impl_->threadHandle();
}
#else
boost::thread::id IOService::threadHandle() {
	return impl_->threadHandle();
}
#endif

#ifdef __USE_PTHREAD__
pthread_t IOService::nativeThreadHandle() {
	return impl_->nativeThreadHandle();
}
#else
boost::thread::native_handle_type IOService::nativeThreadHandle() {
	return impl_->nativeThreadHandle();
}
#endif

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

struct event_base * IOService::coreHandle() {
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
