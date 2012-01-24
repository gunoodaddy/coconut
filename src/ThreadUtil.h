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

#include <assert.h>
#ifdef __USE_PTHREAD__
  #include <pthread.h>
  #define ScopedMutexLock(mutex)	Mutex::ScopedMutexLock(mutex);
#else
  #if ! defined(COCONUT_USE_PRECOMPILE)
    #include <boost/thread/recursive_mutex.hpp>
  #endif
  #define ScopedMutexLock(mutex)	boost::recursive_mutex::scoped_lock autolock(mutex.handle());
#endif

#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/version.hpp>
#endif

#if defined(_MSC_VER)
#pragma warning ( disable: 4231 4251 4275 4786 )
#endif

namespace coconut {

extern bool _activateMultithreadMode_on;

inline boost::uint32_t atomicIncreaseInt32(volatile boost::uint32_t *value) {
#if BOOST_VERSION >= 104800 
	boost::interprocess::ipcdetail::atomic_inc32(value);
#else
	boost::interprocess::detail::atomic_inc32(value);
#endif
	return *value;
}

class COCONUT_API Mutex {
public:
	Mutex() {
#ifdef __USE_PTHREAD__
		assert( pthread_mutexattr_init( &mta_ ) == 0 );
		assert( pthread_mutexattr_settype( &mta_, PTHREAD_MUTEX_RECURSIVE ) == 0 );
		assert( pthread_mutex_init( &mutex_, &mta_ ) == 0 );
#endif
	}

	virtual ~Mutex() {
#ifdef __USE_PTHREAD__
		assert( pthread_mutex_destroy(&mutex_) == 0);
		assert( pthread_mutexattr_destroy(&mta_) == 0 );
#endif
	}

#ifdef __USE_PTHREAD__
	class COCONUT_API ScopedMutexLock {
	public:
		AutoMutexLock(Mutex *mutex) : mutex_(mutex) {
			mutex_->lock();
		}
		~AutoMutexLock() {
			mutex_->unlock();
		}
	private:
		Mutex *mutex_;
	};	
#endif

public:
	int lock() {
		if(_activateMultithreadMode_on) {
#ifdef __USE_PTHREAD__
			return pthread_mutex_lock(&mutex_);
#else
			mutex_.lock();
#endif
		}
		return 0;
	}

	int unlock() {
		if(_activateMultithreadMode_on) {
#ifdef __USE_PTHREAD__
			return pthread_mutex_unlock(&mutex_);
#else
			mutex_.unlock();
#endif
		}
		return 0;
	}

#ifdef __USE_PTHREAD__
	pthread_mutex_t handle() {
		return mutex_;
	}
#else
	boost::recursive_mutex &handle() {
		return mutex_;
	}
#endif

private:
#ifdef __USE_PTHREAD__
	pthread_mutex_t mutex_;
	pthread_mutexattr_t mta_;
#else
	boost::recursive_mutex mutex_;
#endif
};

}
