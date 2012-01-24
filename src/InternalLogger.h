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
#include "Logger.h"

extern volatile bool gEngineLogEnable_; // for performance

// ** Internal Log **
#define _LOG_TRACE(...) \
	if(gEngineLogEnable_) \
		if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_TRACE || coconut::enableDebugMode()) \
			coconut::logger::logprintf(true, __FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_TRACE, __VA_ARGS__); 

#define _LOG_DEBUG(...) \
	if(gEngineLogEnable_) \
		if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_DEBUG || coconut::enableDebugMode()) \
			coconut::logger::logprintf(true, __FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_DEBUG, __VA_ARGS__); 

#define _LOG_INFO(...) \
	if(gEngineLogEnable_) \
		if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_ERROR || coconut::enableDebugMode()) \
			coconut::logger::logprintf(true, __FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_ERROR, __VA_ARGS__); 

#define _LOG_WARN(...) \
	if(gEngineLogEnable_) \
		if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_WARNING || coconut::enableDebugMode()) \
			coconut::logger::logprintf(true, __FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_WARNING, __VA_ARGS__); 

#define _LOG_ERROR(...) \
	if(gEngineLogEnable_) \
		if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_ERROR || coconut::enableDebugMode()) \
			coconut::logger::logprintf(true, __FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_ERROR, __VA_ARGS__); 

#define _LOG_FATAL(...) \
	if(gEngineLogEnable_) \
		if (coconut::logger::currentLogLevel() <= coconut::logger::LEVEL_FATAL || coconut::enableDebugMode()) \
			coconut::logger::logprintf(true, __FILE__, __FUNCTION__, __LINE__, coconut::logger::LEVEL_FATAL, __VA_ARGS__); 
