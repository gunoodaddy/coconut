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

#include "CoconutLib.h"
#include "InternalLogger.h"
#include "ThreadUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#if ! defined(WIN32)
#include <sys/time.h>
#include <execinfo.h>
#endif

volatile bool gEngineLogEnable_ = true;	// extern

namespace coconut { namespace logger {

static LogLevel gCurrentLogLevel_ = LEVEL_TRACE;
static LogHookCallback gLogHookCallback;
static Mutex gLogLock;


void hexdump(const unsigned char *data, const int len, FILE * fp) {           
	const unsigned char *p;
	int i, k;
	
	if (len == 0)
		return;

	p = data;

	fprintf(fp, "%05x     ", 0);
	k = 0;
	for (i = 0; i < len; i++) {
		if (i && !(i % 16)) {
			fprintf(fp, "   ");
			while (k--) {
				if ((int) *p <= 0x20) {
					fprintf(fp, " ");
					p++;
					continue;
				}
				fprintf(fp, "%c", *(p++));
			}
			fprintf(fp, "\n%05x     ", i);
			k = 0;
		}
		fprintf(fp, "%02x%s", (*(p + k) & 0xff), (i%2?" ":"") );
		k++;
	}
	i = 16 - k;
	while (i--)
		fprintf(fp, "   ");
	fprintf(fp, "   ");
	while (k--) {
		if ((int) *p <  0x20) {
			fprintf(fp, ".");
			p++;
			continue;
		}
		fprintf(fp, "%c", *p++);
	}
	fprintf(fp, "\n");
}

void backtrace(FILE * fp) {
#if ! defined(WIN32)
	const size_t max_depth = 100;
	size_t stack_depth;
	void *stack_addrs[max_depth];
	char **stack_strings;

	stack_depth = ::backtrace(stack_addrs, max_depth);
	stack_strings = ::backtrace_symbols(stack_addrs, stack_depth);

	fprintf(fp, "Call stack\n");

	for (size_t i = 1; i < stack_depth; i++) {
		fprintf(fp, "    %s\n", stack_strings[i]);
	}
	free(stack_strings); // malloc()ed by backtrace_symbols
	fflush(fp);
#endif
}

void setEngineLogEnable(bool enable) {
	gEngineLogEnable_ = enable;
}

void setLogLevel(LogLevel level) {
	gCurrentLogLevel_ = level;
}

void setLogHookFunctionCallback(LogHookCallback callback) {
	gLogHookCallback = callback;
}

LogLevel currentLogLevel() {
	return gCurrentLogLevel_; 
}

void logprintf(bool internalLog, const char *file, const char *function, int line, LogLevel level, const char * format, ...) {
	char log[4096] = {0, };
	va_list args;
	va_start(args, format);
	vsprintf(log, format, args);
	va_end(args);

	int len_format = strlen(log);
	if(log[len_format - 1] == '\n')
		log[len_format - 1] = '\0';
	
	logCallbackFunc_t hook = NULL;

	switch(level) {
		case LEVEL_TRACE:
			hook = gLogHookCallback.trace;
			break;
		case LEVEL_DEBUG:
			hook = gLogHookCallback.debug;
			break;
		case LEVEL_INFO:
			hook = gLogHookCallback.info;
			break;
		case LEVEL_WARNING:
			hook = gLogHookCallback.warning;
			break;
		case LEVEL_ERROR:
			hook = gLogHookCallback.error;
			break;
		case LEVEL_FATAL:
			hook = gLogHookCallback.fatal;
			break;
		case LEVEL_NONE:
			return;
	}

	if(hook) {
		hook(level, file, line, function, log, internalLog); 
	} else {
		ScopedMutexLock(gLogLock);
		FILE *fp = stdout;

#if defined(WIN32)
		fprintf(fp, "[%d] > ", (int)level);	
#else
		struct tm * ptm;
		time_t now = time(NULL);
		ptm = localtime(&now);

		struct timeval rv;
		gettimeofday(&rv, NULL);

		fprintf(fp, "[%d:%p]>%02d%02d%02d%02d%02d%02d:%04u ",
				(int)level, (void*)pthread_self(),
				ptm->tm_year - 100, ptm->tm_mon + 1, ptm->tm_mday,
				ptm->tm_hour, ptm->tm_min, ptm->tm_sec, (int)rv.tv_usec);	
#endif
		fprintf(fp, log);
		fprintf(fp, "\n");
		fflush(fp);
	}
}

} }
