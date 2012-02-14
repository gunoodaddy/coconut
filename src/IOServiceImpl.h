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
#include <boost/bind.hpp>
#endif

namespace coconut {

class IOServiceImpl {
public:
	IOServiceImpl() 
		: ioServiceContainer_(NULL)
		, id_(0)
		, multithread_(false)
		, finalizedFlag_(false)
		, joinedThreadFlag_(false)
#if defined(WIN32)
		, cpuCnt_(0)
		, enabledIOCP_(false)
#endif
		, thread_() { }

	IOServiceImpl(int id, BaseIOServiceContainer *ioServiceContainer, bool threadMode) 
		: ioServiceContainer_(ioServiceContainer)
		, id_(id)
		, multithread_(threadMode)
		, finalizedFlag_(false)
		, joinedThreadFlag_(false)
#if defined(WIN32)
		, cpuCnt_(0)
		, enabledIOCP_(false)
#endif
		, thread_() { }

	virtual ~IOServiceImpl() { }

public:
	boost::thread::id threadHandle() {
		if(!multithread_)
			return boost::this_thread::get_id();
		return thread_.get_id();
	}

	boost::thread::native_handle_type nativeThreadHandle() {
		return thread_.native_handle();
	}

	bool isCalledInMountedThread() {
		if(!multithread_) {
			return true;	// always true
		}
		return threadHandle() == boost::this_thread::get_id();
	}

	void deferredCall(IOService::deferredMethod_t func) {
		lockDeferredCaller_.lock();
		deferredCallbacks_.push_back(func);
		lockDeferredCaller_.unlock();

		// this function must be called last in this function for preventing from race-condition.
		triggerDeferredEvent();
	}

	int id() {
		return id_;
	}

	BaseIOServiceContainer *ioServiceContainer() {
		return ioServiceContainer_;
	}

	Mutex &mutex() {
		return lock_;
	}

	void _startEventLoopBlock() {
		dispatchEvent();
	}

	void _startEventLoopInThread() {
		thread_ = boost::thread(boost::bind(&IOServiceImpl::dispatchEvent, this));
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

	int _joinThread() {
		if(multithread_ && false == joinedThreadFlag_) {
			int ret = 0;
			thread_.join();
			joinedThreadFlag_ = true;
			_LOG_DEBUG("joined thread.. this = %p", this);
			return ret;
		}
		return 0;
	}

	void initialize() {
#if defined(WIN32)	
		if(false == gStartUpWinSock) {
			WSADATA wsaData;
			::WSAStartup(MAKEWORD(2, 2), &wsaData);
			gStartUpWinSock = true;
		}
#endif
		createHandle();
	}

	void finalize() {
		if(finalizedFlag_)
			return;
		finalizedFlag_ = true;

		stop();
		_joinThread();

		destoryHandle();
	}

	virtual void initialize(int id, BaseIOServiceContainer *ioServiceContainer, bool threadMode) = 0;
	virtual coconut_io_handle_t coreHandle() = 0;
	virtual void stop() = 0;
	virtual void dispatchEvent() = 0;
	virtual void createHandle() = 0;
	virtual void destoryHandle() = 0;
	virtual void triggerDeferredEvent() = 0;
	virtual bool isStopped() = 0;

protected:
	void fireDeferredEvent() {
		lockDeferredCaller_.lock();
		for(size_t i = 0; i < deferredCallbacks_.size(); i++) {
			deferredCallbacks_[i]();
		}
		deferredCallbacks_.clear();
		lockDeferredCaller_.unlock();
	}

protected:
	BaseIOServiceContainer *ioServiceContainer_;
	int id_;
	volatile bool multithread_;
	volatile bool finalizedFlag_;
	volatile bool joinedThreadFlag_;
#if defined(WIN32)
	size_t cpuCnt_;	// for iocp
	bool enabledIOCP_;
#endif
	boost::thread thread_;
	
	Mutex lock_;
	Mutex lockDeferredCaller_;
	std::vector<IOService::deferredMethod_t> deferredCallbacks_;
};

}
