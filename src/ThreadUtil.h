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
#if ! defined(COCONUT_USE_PRECOMPILE)
 #include <boost/thread/recursive_mutex.hpp>
#endif
//#define ScopedMutexLock(mutex)	boost::recursive_mutex::scoped_lock autolock(mutex.handle());
#define ScopedMutexLock(mutex)	coconut::Mutex::ScopedLock autolock(&mutex)

#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/version.hpp>
#endif

#if defined(_MSC_VER)
#pragma warning ( disable: 4231 4251 4275 4786 )
#endif

namespace coconut {

extern bool _activateMultithreadMode_on;

inline boost::uint32_t atomicIncreaseInt32(volatile boost::uint32_t *mem) {
#if BOOST_VERSION >= 104800 
	boost::interprocess::ipcdetail::atomic_inc32(mem);
#else
	boost::interprocess::detail::atomic_inc32(mem);
#endif
	return *mem;
}

inline void atomicWriteInt32(volatile boost::uint32_t *mem, boost::uint32_t val) {
#if BOOST_VERSION >= 104800 
	boost::interprocess::ipcdetail::atomic_write32(mem, val);
#else
	boost::interprocess::detail::atomic_write32(mem, val);
#endif
}



class COCONUT_API Mutex {
public:
	Mutex() { }

	virtual ~Mutex() { }

	class ScopedLock
	{
		Mutex* m;
		bool locked;
		public:
		explicit ScopedLock(Mutex* m_):
			m(m_),locked(true)
		{
			m->lock();
		}
		void unlock()
		{
			m->unlock();
			locked=false;
		}

		~ScopedLock()
		{
			if(locked)
			{
				m->unlock();
			}
		}
	};

public:
	inline int lock() {
		if(_activateMultithreadMode_on) {
			mutex_.lock();
		}
		return 0;
	}

	inline int unlock() {
		if(_activateMultithreadMode_on) {
			mutex_.unlock();
		}
		return 0;
	}

	boost::recursive_mutex &handle() {
		return mutex_;
	}

private:
	boost::recursive_mutex mutex_;
};

}
