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

#if defined(_MSC_VER)
#pragma warning ( disable: 4231 4251 4275 4786 )
#endif

namespace coconut {

extern bool _activateMultithreadMode_on;

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
