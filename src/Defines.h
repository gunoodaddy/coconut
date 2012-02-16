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
#if defined(HAVE_SYS_QUEUE_H)
#include <sys/queue.h>
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

typedef void * coconut_io_handle_t;
typedef void * coconut_http_request_handle_t;

#define MAKE_TIMEVAL_SEC(SEC) MAKE_TIMEVAL_MSEC(SEC*1000)
#define MAKE_TIMEVAL_MSEC(MSEC) {MSEC/1000, (MSEC % 1000) * 1000}

#define IOBUF_LEN	16*1024

namespace coconut {

typedef std::vector<std::string> stringlist_t;

extern bool _setLittleEndian_on;

enum ThreadPriority {
	LOWEST = 0,
	LOWER = 1,
	LOW = 2,
	NORMAL = 3,
	HIGH = 4,
	HIGHER = 5,
	HIGHEST = 6
};

enum SocketType{
	TCP,
	UDP,
};

enum HttpMethodType {
	HTTP_POST,
	HTTP_GET,
	HTTP_UNKNOWN
};

#ifndef TAILQ_INIT
#define	TAILQ_INIT(head) do {						\
	(head)->tqh_first = NULL;					\
	(head)->tqh_last = &(head)->tqh_first;				\
} while (/*CONSTCOND*/0)
#endif

#ifndef TAILQ_FIRST
#define TAILQ_FIRST(head)       ((head)->tqh_first)
#endif
#ifndef TAILQ_END
#define TAILQ_END(head)         NULL
#endif
#ifndef TAILQ_NEXT
#define TAILQ_NEXT(elm, field)      ((elm)->field.tqe_next)
#endif

#ifndef TAILQ_FOREACH
#define TAILQ_FOREACH(var, head, field)                 \
	for ((var) = TAILQ_FIRST(head);                 \
			(var) != TAILQ_END(head);                  \
			(var) = TAILQ_NEXT(var, field))
#endif


} // end of namespace coconut
