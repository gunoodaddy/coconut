#include "Coconut.h"
#include "IOService.h"
#include "Exception.h"
#include "Logger.h"
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

class DefaultIOServiceContainer;
boost::shared_ptr<DefaultIOServiceContainer> globalDefaultIOServiceContainer_;

#ifdef __USE_PTHREAD__
static int set_thread_priority(pthread_attr_t *thread_attr, ThreadPriority priority) {
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

	boost::shared_ptr<IOService> ioService() { return ioService_; }
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


class IOServiceImpl {
public:
	IOServiceImpl(BaseIOServiceContainer *ioServiceContainer, bool threadMode) : ioServiceContainer_(ioServiceContainer)
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
		LOG_TRACE("~IOServiceImpl() this = %p", this);
	}

public:
#ifdef __USE_PTHREAD__
	pthread_t threadHandle() {
		return thread_;
	}

#else
	boost::thread::id threadHandle() {
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
#ifdef __USE_PTHREAD__
		return threadHandle() == pthread_self();
#else
		return threadHandle() == boost::this_thread::get_id();
#endif
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

		event_ = event_new(base_, -1, EV_READ|EV_PERSIST, null_event_cb, this);
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
			LOG_DEBUG("IOService stop eventloop..");
			struct timeval tv = MAKE_TIMEVAL_MSEC(10);
			event_base_loopexit(base_, &tv);
			loopExitFlag_ = true;
			//event_base_loopbreak(base_);
		}
	}

	void _startEventLoopBlock() {
		dispatchEvent();
	}

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
		if(set_thread_priority(&thread_attr, priority_) != 0) {
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
			LOG_INFO("joined thread.. this = %p", this);
#endif
			return ret;
		}
		return 0;
	}

	void dispatchEvent() {
		event_base_dispatch(base_);

		LOG_INFO("finished dispatch event.. this = %p", this);
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

	static void null_event_cb(coconut_socket_t fd, short what, void *arg) {
		// NOTHING TO DO.. FOR MULTITHREAD NULL-EVENT..
	}


private:
	BaseIOServiceContainer *ioServiceContainer_;
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
	struct event *event_;
};



//--------------------------------------------------------------------------------------------------

IOService::IOService() {
	BaseIOServiceContainer *ioServiceContainer = DefaultIOServiceContainer::instance(shared_from_this());
	impl_ = new IOServiceImpl(ioServiceContainer, false);
}

IOService::IOService(BaseIOServiceContainer *ioServiceContainer, bool threadMode) {
	if(NULL == ioServiceContainer)
		ioServiceContainer = DefaultIOServiceContainer::instance(shared_from_this());
		
	impl_ = new IOServiceImpl(ioServiceContainer, threadMode);
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
