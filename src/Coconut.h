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
