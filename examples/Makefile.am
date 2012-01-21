
DEFAULT_INCLUDES = -I. -I.. -I$(top_builddir)/src

LDADD = $(top_builddir)/src/libcoconut.la $(log4cxx_LIBS) -lboost_thread

AM_CPPFLAGS = $(BOOST_CPPFLAGS)
AM_CXXFLAGS = -Wall
AM_LDFLAGS = $(BOOST_LDFLAGS) -static

noinst_PROGRAMS = \
	tcpserver \
	tcpclient \
	udpserver \
	udpclient \
	udsserver \
	udsclient \
	httpserver \
	fdserver \
	fdclient

tcpserver_SOURCES = \
	tcpserver.cpp

tcpclient_SOURCES = \
	tcpclient.cpp

udpserver_SOURCES = \
	udpserver.cpp

udpclient_SOURCES = \
	udpclient.cpp

udsserver_SOURCES = \
	udsserver.cpp

udsclient_SOURCES = \
	udsclient.cpp

fdserver_SOURCES = \
	fdserver.cpp

fdclient_SOURCES = \
	fdclient.cpp

httpserver_SOURCES = \
	httpserver.cpp

CLEANFILES=core* tag cscope.* *.log *.log.* .*.sw* *~ result *.sock
DISTCLEANFILES=$(CLEANFILES)
