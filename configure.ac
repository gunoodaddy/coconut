#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([coconut], [0.0.1], [gunoodaddy@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

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
AX_LOG4CXX(, [AC_MSG_ERROR([log4cxx not found])])

AC_CHECK_LIB(event, event_base_dispatch, [], AC_MSG_ERROR([event library not found]))
AC_CHECK_LIB(event_pthreads, evthread_use_pthreads, [], AC_MSG_ERROR([event_pthread library not found]))
AC_CHECK_LIB(hiredis, redisAsyncConnect, [], AC_MSG_ERROR([hiredis library not found]))
AC_CHECK_LIB(pthread, pthread_create, [], AC_MSG_ERROR([pthread library not found]))

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
                 tester/Makefile
				 examples/Makefile])
AC_OUTPUT
