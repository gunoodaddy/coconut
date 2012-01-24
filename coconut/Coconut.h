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

#include "config.h"
#include <vector>
#include <string>

#if defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/cstdint.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#endif


#if defined(WIN32)
#if defined(COCONUT_STATIC)
#define COCONUT_API
//   definitions used when building DLL
#elif defined(COCONUT_EXPORTS)
#define COCONUT_API __declspec(dllexport)
#else
//    definitions used when using DLL
#define COCONUT_API __declspec(dllimport)
#endif
#else
#define COCONUT_API 
#endif

// ** Compatibility **
#define COOKIE_INVALID_SOCKET	0
#if defined(WIN32)
#define COOKIE_ETIMEDOUT	WSAETIMEDOUT
#else
#define COOKIE_ETIMEDOUT	ETIMEDOUT
#endif

#if defined(WIN32)
typedef intptr_t coconut_socket_t;
#else
typedef int coconut_socket_t;
#endif

#define MAKE_TIMEVAL_SEC(SEC) MAKE_TIMEVAL_MSEC(SEC*1000)
#define MAKE_TIMEVAL_MSEC(MSEC) {MSEC/1000, (MSEC % 1000) * 1000}

#define IOBUF_LEN	16*1024

namespace coconut {

typedef std::vector<std::string> stringlist_t;

extern bool _setEnableDebugMode_on;

enum ThreadPriority {
	LOWEST = 0,
	LOWER = 1,
	LOW = 2,
	NORMAL = 3,
	HIGH = 4,
	HIGHER = 5,
	HIGHEST = 6
};

enum HttpMethodType {
	HTTP_POST,
	HTTP_GET
};

enum SocketType{
	TCP,
	UDP,
};

COCONUT_API void setEnableDebugMode();
COCONUT_API bool enableDebugMode();

} // end of namespace coconut
