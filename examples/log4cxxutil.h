#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <log4cxx/logger.h> 
#include <log4cxx/helpers/optionconverter.h>

// ** Log **
#define MY_LOG4CXX_TRACE(logger, ...) \
	if (LOG4CXX_UNLIKELY(logger->isTraceEnabled()) || coconut::enableDebugMode()) \
		log4cxx_printf(logger, __FILE__, __FUNCTION__, __LINE__, ::log4cxx::Level::getTrace(), __VA_ARGS__); 

#define MY_LOG4CXX_DEBUG(logger, ...) \
	if (LOG4CXX_UNLIKELY(logger->isDebugEnabled()) || coconut::enableDebugMode()) \
		log4cxx_printf(logger, __FILE__, __FUNCTION__, __LINE__, ::log4cxx::Level::getDebug(), __VA_ARGS__); 

#define MY_LOG4CXX_INFO(logger, ...) \
	if (LOG4CXX_UNLIKELY(logger->isInfoEnabled()) || coconut::enableDebugMode()) \
		log4cxx_printf(logger, __FILE__, __FUNCTION__, __LINE__, ::log4cxx::Level::getInfo(), __VA_ARGS__); 

#define MY_LOG4CXX_WARN(logger, ...) \
	if (LOG4CXX_UNLIKELY(logger->isWarnEnabled()) || coconut::enableDebugMode()) \
		log4cxx_printf(logger, __FILE__, __FUNCTION__, __LINE__, ::log4cxx::Level::getWarn(), __VA_ARGS__); 

#define MY_LOG4CXX_ERROR(logger, ...) \
	if (LOG4CXX_UNLIKELY(logger->isErrorEnabled()) || coconut::enableDebugMode()) \
		log4cxx_printf(logger, __FILE__, __FUNCTION__, __LINE__, ::log4cxx::Level::getError(), __VA_ARGS__); 

#define MY_LOG4CXX_ASSERT(logger, ...) \
	if (!(condition) && logger->isErrorEnabled() || coconut::enableDebugMode()) {\
		log4cxx_printf(logger, __FILE__, __FUNCTION__, __LINE__, ::log4cxx::Level::getError(), __VA_ARGS__); 

#define MY_LOG4CXX_FATAL(logger, ...) \
	if (LOG4CXX_UNLIKELY(logger->isFatalEnabled()) || coconut::enableDebugMode()) \
		log4cxx_printf(logger, __FILE__, __FUNCTION__, __LINE__, ::log4cxx::Level::getFatal(), __VA_ARGS__); 


void log4cxx_printf(log4cxx::LoggerPtr logger, const char *file, const char *function, int line, const log4cxx::LevelPtr& level, const char * format, ...) {

	char log[1024] = {0, };
	va_list args;
	va_start(args, format);
	vsprintf(log, format, args);
	va_end(args);

	int len_format = strlen(log);
	if(log[len_format - 1] == '\n')
		log[len_format - 1] = '\0';
	
	::log4cxx::helpers::MessageBuffer oss_; 
	::log4cxx::spi::LocationInfo location(file, function, line);
	logger->forcedLog(level, oss_.str(oss_ << log), location); 
}

