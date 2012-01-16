
DEFAULT_INCLUDES = -I.. -I. -I$(top_builddir)/src

LDADD = $(top_builddir)/src/libcoconut.la $(log4cxx_LIBS) -lboost_thread -llog4cxx

AM_CPPFLAGS = $(BOOST_CPPFLAGS)
AM_CXXFLAGS = -Wall
AM_LDFLAGS = $(BOOST_LDFLAGS) -static

noinst_PROGRAMS = \
	tester \
	boosttest \
	unittest

tester_SOURCES = \
	tester.cpp

boosttest_SOURCES = \
	boosttest.cpp

unittest_SOURCES = \
	unittest.cpp

CLEANFILES=core* tag cscope.* *.log *.log.* .*.sw* *~ result *.sock
DISTCLEANFILES=$(CLEANFILES)
