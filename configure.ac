#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([coconut], [0.0.1], [gunoodaddy@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_LANG(C++)

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LIBTOOL

# Checks for libraries.
AX_BOOST_BASE([1.32.0], , [AC_MSG_ERROR([Boost C++ Libraries are not installed, but are required by libbgpparser])])
AX_LIB_EVENT([2.0.15], , [AC_MSG_ERROR([libevent not found])])

AC_LANG(C++)
#AC_CHECK_LIB(jemalloc, malloc_stats_print, [], [])
AC_CHECK_LIB(tcmalloc, tc_cfree , [], [])
AC_CHECK_LIB(event, event_base_dispatch, [], AC_MSG_ERROR([event library not found]))
AC_CHECK_LIB(event_pthreads, evthread_use_pthreads, [], AC_MSG_ERROR([event_pthread library not found]))
AC_CHECK_LIB(event, evhttp_request_set_free_cb, 
	AC_DEFINE(HAVE_LIBEVENT_GUNOODADDY_FIX,,[define if libevent library fixed by gunoodaddy is available]), [])
AC_CHECK_LIB(pthread, pthread_create, [], AC_MSG_ERROR([pthread library not found]))

# Check for redis
AC_ARG_WITH([redis],
		AS_HELP_STRING([--without-redis], [disable hiredis (default is no)]),
		[with_redis=no],
		[with_redis=yes])
AS_IF([test "x$with_redis" != xno],
		AC_CHECK_LIB(hiredis, redisAsyncConnect, [], AC_MSG_ERROR([hiredis library not found])))

# Check for json
AC_ARG_WITH([json],
		AS_HELP_STRING([--without-json], [disable json feature with libjson (default is no)]),
		[],
		AC_DEFINE(HAVE_LIBJSON,,[define if --without-json option is not set]))

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netinet/in.h stdlib.h string.h sys/socket.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday inet_ntoa memset socket strdup])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile
				 examples/Makefile])
AC_OUTPUT
