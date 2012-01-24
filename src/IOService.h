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
