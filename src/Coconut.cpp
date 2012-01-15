#include "Coconut.h"
#include <event2/thread.h>
#include <fcntl.h>
#include <assert.h>

namespace coconut {

bool _setEnableDebugMode_on = false;
bool _activateMultithreadMode_on = false;

COCONUT_API bool enableDebugMode() {
	return _setEnableDebugMode_on;
}

COCONUT_API void setEnableDebugMode() {
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

}
