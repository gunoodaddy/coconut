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

#include <event2/event.h>
#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/bind.hpp>
#endif
#include "IOServiceImpl.h"
#include "BaseObjectAllocator.h"

namespace coconut {

#if defined(WIN32)
extern void activateMultithreadMode();
#endif

class LibeventIOServiceImpl : public IOServiceImpl 
							, public BaseObjectAllocator<LibeventIOServiceImpl>
{
public:
	LibeventIOServiceImpl()
		: IOServiceImpl()
		, loopExitFlag_(false)
		, base_cfg_(NULL)
		, base_(NULL)
		, event_(NULL) { }

	LibeventIOServiceImpl(int id, BaseIOServiceContainer *ioServiceContainer, bool threadMode) 
		: IOServiceImpl(id, ioServiceContainer, threadMode)
		, loopExitFlag_(false)
		, base_cfg_(NULL)
		, base_(NULL)
		, event_(NULL) { }

	~LibeventIOServiceImpl() {
		finalize();
		_LOG_TRACE("~LibeventIOServiceImpl() this = %p", this);
	}

	void initialize(int id, BaseIOServiceContainer *ioServiceContainer, bool threadMode) {
		id_ = id;
		ioServiceContainer_ = ioServiceContainer;
		multithread_ = threadMode;
	}

public:
	coconut_io_handle_t coreHandle() {
		return (coconut_io_handle_t)base_;
	}
	
	void triggerDeferredEvent() {
		event_active(event_, EV_READ, 1);
	}

	void createHandle() {
#if defined(WIN32)	
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

	void destoryHandle() {
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

	void stop() {
		if(false == loopExitFlag_) {
			_LOG_DEBUG("IOService stop eventloop..");
			struct timeval tv = MAKE_TIMEVAL_MSEC(10);
			event_base_loopexit(base_, &tv);
			loopExitFlag_ = true;
			//event_base_loopbreak(base_);
		}
	}

	void dispatchEvent() {
		event_base_dispatch(base_);

		_LOG_DEBUG("finished dispatch event.. this = %p", this);
		// TODO gracefully program exit logic need...
		//assert(false && "event loop exit???? why?");
	}

	inline bool isStopped() {
		return loopExitFlag_;
	}

private:
	static void deferred_event_cb(coconut_socket_t fd, short what, void *arg) {
		LibeventIOServiceImpl *SELF = (LibeventIOServiceImpl *)arg;
		SELF->fireDeferredEvent();
	}

private:
	volatile bool loopExitFlag_;
	struct event_config * base_cfg_;
	struct event_base * base_;
	struct event *event_;
};

}
