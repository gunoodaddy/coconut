#include "Coconut.h"
#include "Logger.h"
#include "ThreadUtil.h"
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>

namespace coconut { namespace logger {

static LogLevel gCurrentLogLevel_ = LEVEL_TRACE;
static LogHookCallback gLogHookCallback;
static Mutex gLogLock;

void hexdump(const unsigned char *data, const int len, FILE * fp) {           
	const unsigned char *p;
	int i, k;
	
	if(_setEnableDebugMode_on == false)
		return;

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


void setLogLevel(LogLevel level) {
	gCurrentLogLevel_ = level;
}

void setLogHookFunctionCallback(LogHookCallback callback) {
	gLogHookCallback = callback;
}

LogLevel currentLogLevel() {
	return gCurrentLogLevel_; 
}

void logprintf(const char *file, const char *function, int line, LogLevel level, const char * format, ...) {
	char log[1024] = {0, };
	va_list args;
	va_start(args, format);
	vsprintf(log, format, args);
	va_end(args);

	int len_format = strlen(log);
	if(log[len_format - 1] == '\n')
		log[len_format - 1] = '\0';
	
	void (*hook)(LogLevel level, const char *fileName, int fileLine, const char *functionName, const char *logmsg) = NULL;

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
	}

	if(hook) {
		hook(level, file, line, function, log); 
	} else {
		ScopedMutexLock(gLogLock);
		va_list args;
		struct tm * ptm;
		time_t now = time(NULL);
		ptm = localtime(&now);

		struct timeval rv;
		gettimeofday(&rv, NULL);

		FILE *fp = stdout;

		va_start(args, format);

		fprintf(fp, "[%p]> %02d%02d%02d%02d%02d%02d:%03u ",
				(void*)pthread_self(),
				ptm->tm_year - 100, ptm->tm_mon + 1, ptm->tm_mday,
				ptm->tm_hour, ptm->tm_min, ptm->tm_sec, (int)rv.tv_usec);
		vfprintf(fp, format, args);

		int len_format = strlen(format);
		if(format[len_format - 1] != '\n')
			fprintf(fp, "\n");
		fflush(fp);

		va_end(args);
	}
}

} }
