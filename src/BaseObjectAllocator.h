#pragma once

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

#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#endif

namespace coconut {

template <class T>
class BaseObjectAllocator
{
public:
	BaseObjectAllocator() { }
	~BaseObjectAllocator() { }

	class Allocator {
	public:
		Allocator() {}
		virtual ~Allocator() {}

		virtual T* make() = 0;
		virtual void destroy(T *p) = 0;
		virtual boost::shared_ptr<T> makeSharedPtr() = 0;
	};
	
	typedef boost::shared_ptr<Allocator> ALLOCATOR;
	typedef boost::function< void (T *) > DESTRUCTOR_FUNC;

	static inline void setAllocator(boost::shared_ptr<Allocator> alloc) {
		allocator_ = alloc;	
		destructSharedPtr_ = boost::bind(&Allocator::destroy, allocator_, _1);
	}

	static inline boost::shared_ptr<T> makeSharedPtr() {
		if(allocator_)
			return allocator_->makeSharedPtr();
		return boost::shared_ptr<T>(new T);
	}

	static inline T* make() {
		if(allocator_)
			return allocator_->make();
		return new T();
	}

	static inline void destroy(T *p) {
		if(!p) return;
		if(allocator_)
			return allocator_->destroy(p);
		delete p;
	}

	static inline DESTRUCTOR_FUNC destroyFunction() {
		return destructSharedPtr_;
	}

private:
	static ALLOCATOR allocator_;
	static DESTRUCTOR_FUNC destructSharedPtr_;
};

template <class T> typename BaseObjectAllocator<T>::ALLOCATOR BaseObjectAllocator<T>::allocator_;
template <class T> typename BaseObjectAllocator<T>::DESTRUCTOR_FUNC BaseObjectAllocator<T>::destructSharedPtr_;

}
