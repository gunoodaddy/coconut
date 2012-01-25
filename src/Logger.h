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
#include <stdio.h>

// ** Log **
#define LOG_TRACE(...) \
	if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_TRACE || coconut::enableDebugMode()) \
		coconut::logger::logprintf(false, __FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_TRACE, __VA_ARGS__); 

#define LOG_DEBUG(...) \
	if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_DEBUG || coconut::enableDebugMode()) \
		coconut::logger::logprintf(false, __FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_DEBUG, __VA_ARGS__); 

#define LOG_INFO(...) \
	if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_ERROR || coconut::enableDebugMode()) \
		coconut::logger::logprintf(false, __FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_ERROR, __VA_ARGS__); 

#define LOG_WARN(...) \
	if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_WARNING || coconut::enableDebugMode()) \
		coconut::logger::logprintf(false, __FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_WARNING, __VA_ARGS__); 

#define LOG_ERROR(...) \
	if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_ERROR || coconut::enableDebugMode()) \
		coconut::logger::logprintf(false, __FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_ERROR, __VA_ARGS__); 

#define LOG_FATAL(...) \
	if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_FATAL || coconut::enableDebugMode()) \
		coconut::logger::logprintf(false, __FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_FATAL, __VA_ARGS__); 

namespace coconut { namespace logger {

enum LogLevel{
	LEVEL_TRACE = 1,
	LEVEL_DEBUG,
	LEVEL_INFO,
	LEVEL_WARNING,
	LEVEL_ERROR,
	LEVEL_FATAL,
	LEVEL_NONE,
};

/* Log Hook Callback */
typedef void (*logCallbackFunc_t) (LogLevel level, const char *fileName, int fileLine, const char *functionName, const char *logmsg, bool internalLog);

typedef struct COCONUT_API LogHookCallback {
	logCallbackFunc_t trace;
	logCallbackFunc_t debug;
	logCallbackFunc_t info;
	logCallbackFunc_t warning;
	logCallbackFunc_t error;
	logCallbackFunc_t fatal;
}LogHookCallback;

COCONUT_API void setEngineLogEnable(bool enable);	// default enable

COCONUT_API void setLogLevel(LogLevel level);

COCONUT_API void setLogHookFunctionCallback(LogHookCallback callback);

COCONUT_API LogLevel currentLogLevel();

COCONUT_API void logprintf( bool internalLog, 
							const char *file, 
							const char *function, 
							int line, LogLevel level, 
							const char * format, ...);

COCONUT_API void hexdump(const unsigned char *data, const int len, FILE * fp);
COCONUT_API void backtrace(FILE * fp);

} }  // end of namespace coconut / logger
