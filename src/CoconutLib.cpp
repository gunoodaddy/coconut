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
#include <event2/thread.h>
#include <fcntl.h>
#include <assert.h>

namespace coconut {

bool _startUpWinSock = false;
bool _activateMultithreadMode_on = false;
bool _setLittleEndian_on = false;
bool _setEnableDebugMode_on = false;

bool enableDebugMode() {
	return _setEnableDebugMode_on;
}

void setEnableDebugMode() {
	_setEnableDebugMode_on = true;
}

/*
size_t get_n_bytes_readable_on_socket(evutil_socket_t fd)
{
#define BUFFER_MAX_READ   4096
#if defined(FIONREAD) && defined(WIN32)
	unsigned long lng = BUFFER_MAX_READ;
	if (ioctlsocket(fd, FIONREAD, &lng) < 0)
		return 0;
	return (int)lng;
#elif defined(FIONREAD)
	int n = BUFFER_MAX_READ;
	if (ioctl(fd, FIONREAD, &n) < 0)
		return 0;
	return n;
#else
	return BUFFER_MAX_READ;
#endif
}
*/
void activateMultithreadMode() {
#if defined(WIN32)
	evthread_use_windows_threads();
#else
	evthread_use_pthreads();
#endif
	
	_activateMultithreadMode_on = true;
}

void setUseLittleEndianForNetwork(bool littenEndian /* default is falese */) {
	_setLittleEndian_on = littenEndian;
}

}
