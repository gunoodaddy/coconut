#pragma once
#include <stdio.h>

// ** Log **
#define LOG_TRACE(...) \
	if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_TRACE || coconut::enableDebugMode()) \
		coconut::logger::logprintf(__FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_TRACE, __VA_ARGS__); 

#define LOG_DEBUG(...) \
	if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_DEBUG || coconut::enableDebugMode()) \
		coconut::logger::logprintf(__FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_DEBUG, __VA_ARGS__); 

#define LOG_INFO(...) \
	if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_ERROR || coconut::enableDebugMode()) \
		coconut::logger::logprintf(__FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_ERROR, __VA_ARGS__); 

#define LOG_WARN(...) \
	if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_WARNING || coconut::enableDebugMode()) \
		coconut::logger::logprintf(__FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_WARNING, __VA_ARGS__); 

#define LOG_ERROR(logger, ...) \
	if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_ERROR || coconut::enableDebugMode()) \
		coconut::logger::logprintf(__FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_ERROR, __VA_ARGS__); 

#define LOG_FATAL(logger, ...) \
	if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_FATAL || coconut::enableDebugMode()) \
		coconut::logger::logprintf(__FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_FATAL, __VA_ARGS__); 

namespace coconut { namespace logger {

enum LogLevel{
	LEVEL_TRACE,
	LEVEL_DEBUG,
	LEVEL_INFO,
	LEVEL_WARNING,
	LEVEL_ERROR,
	LEVEL_FATAL,
};

/* Log Hook Callback */
struct LogHookCallback {
	void (*trace)  (const char *fileName, int fileLine, const char *functionName, const char *logmsg);
	void (*debug)  (const char *fileName, int fileLine, const char *functionName, const char *logmsg);
	void (*info)   (const char *fileName, int fileLine, const char *functionName, const char *logmsg);
	void (*warning)(const char *fileName, int fileLine, const char *functionName, const char *logmsg);
	void (*error)  (const char *fileName, int fileLine, const char *functionName, const char *logmsg);
	void (*fatal)  (const char *fileName, int fileLine, const char *functionName, const char *logmsg);
};

void setLogLevel(LogLevel level);
void setLogHookFunctionCallback(struct LogHookCallback &callback);
LogLevel currentLogLevel();

void logprintf(const char *file, const char *function, int line, LogLevel level, const char * format, ...);

void hexdump(const unsigned char *data, const int len, FILE * fp);

} }  // end of namespace coconut / logger
