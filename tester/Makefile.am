
DEFAULT_INCLUDES = -I.. -I. -I$(top_builddir)/src

LDADD = $(top_builddir)/src/libcoconut.la $(log4cxx_LIBS) -lboost_thread -ljson

AM_CPPFLAGS = $(BOOST_CPPFLAGS)
AM_CXXFLAGS = -Wall
AM_LDFLAGS = $(BOOST_LDFLAGS) -static

noinst_PROGRAMS = \
	unittest \
	boosttest \
	jsontest

boosttest_SOURCES = \
	boosttest.cpp

jsontest_SOURCES = \
	jsontest.cpp

unittest_SOURCES = \
	unittest.cpp

CLEANFILES=core* tag cscope.* *.log *.log.* .*.sw* *~ result *.sock
DISTCLEANFILES=$(CLEANFILES)
