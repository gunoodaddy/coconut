#pragma once

#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>
#endif

#ifdef __USE_PTHREAD__
#include <pthread.h>
#endif
#include "ThreadUtil.h"

#define ScopedIOServiceLock(ioService)	ScopedMutexLock(ioService->mutex())

#define CHECK_IOSERVICE_STOP_VOID_RETURN(ioService)	\
	if(ioService->isStopped())	\
		return;

struct event_base;

namespace coconut {
class BaseIOServiceContainer;
class IOServiceImpl;

class COCONUT_API IOService : public boost::enable_shared_from_this<IOService> {
public:
	IOService();
	~IOService();

private:
	IOService(BaseIOServiceContainer *ioServiceContainer, bool threadMode = false);

public:
#ifdef __USE_PTHREAD__
	pthread_t threadHandle();
#else
	boost::thread::id threadHandle();
#endif

#ifdef __USE_PTHREAD__
	pthread_t nativeThreadHandle();
#else
	boost::thread::native_handle_type nativeThreadHandle();
#endif

	void initialize();
	void run();
	void stop();

	bool isCalledInMountedThread();
	bool isStopped();

	BaseIOServiceContainer *ioServiceContainer();
	struct event_base * coreHandle();
	Mutex &mutex();

#if defined(WIN32)
	void turnOnIOCP(size_t cpuCount);	// must call before initialize()
#endif
private:
	void runWithoutJoinThread();
	void joinThread();
	
private:
	friend class IOServiceContainer;
	IOServiceImpl *impl_;
};

}
