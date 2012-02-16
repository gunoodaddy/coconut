lib_LTLIBRARIES = libcoconut.la

AM_CXXFLAGS = -g -Wall
AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(srcdir)/
AM_LDFLAGS = $(BOOST_LDFLAGS)

libcoconut_la_LIBADD = -lboost_thread -ljson

libcoconut_la_SOURCES = \
                       dep/kbuffer.c \
                       dep/MPFDParser/Exception.cpp \
                       dep/MPFDParser/Field.cpp \
                       dep/MPFDParser/Parser.cpp \
                       CoconutLib.cpp \
                       Logger.cpp \
                       IOService.cpp \
                       IOServiceContainer.cpp \
					   DeferredCaller.cpp \
                       Timer.cpp \
                       DNSResolver.cpp \
                       BufferedTransport.cpp \
                       BaseSocket.cpp \
                       TcpSocket.cpp \
                       UdpSocket.cpp \
                       ConnectionListener.cpp \
                       RedisRequest.cpp \
                       RedisResponse.cpp \
                       HttpClient.cpp \
                       HttpRequest.cpp \
                       HttpServer.cpp \
                       BaseProtocol.cpp \
                       FrameProtocol.cpp \
                       LineProtocol.cpp \
                       JSONProtocol.cpp \
                       FileDescriptorProtocol.cpp \
                       BaseController.cpp \
                       BaseControllerEvent.cpp \
                       ClientController.cpp \
                       ServerController.cpp  \
                       LineController.cpp \
                       JSONController.cpp \
                       FrameController.cpp \
                       FileDescriptorController.cpp \
                       NetworkHelper.cpp \
					   BaseIOSystemFactory.cpp

include_coconutdir = $(includedir)/coconut
include_coconut_HEADERS = \
                         Coconut.h \
						 Logger.h \
                         ThreadUtil.h \
                         Exception.h \
                         BaseIOServiceContainer.h \
                         BaseSocket.h \
						 BaseProtocol.h \
                         BaseController.h \
                         BaseControllerEvent.h \
                         IOService.h \
                         IOServiceContainer.h \
						 DeferredCaller.h \
                         Timer.h \
                         DNSResolver.h \
                         BaseVirtualTransport.h \
                         BufferedTransport.h \
						 VirtualTransportHelper.h \
                         TcpSocket.h \
                         UdpSocket.h \
                         ConnectionListener.h \
                         RedisRequest.h \
                       	 RedisResponse.h \
                         HttpClient.h \
                         HttpServer.h \
						 JSONProtocol.h \
						 LineProtocol.h \
						 FrameProtocol.h \
                         FileDescriptorProtocol.h \
                         ClientController.h \
                         LineController.h \
                         JSONController.h \
                         FrameController.h \
                         FileDescriptorController.h \
                         ServerController.h \
                         NetworkHelper.h \
						 BaseObjectAllocator.h \
						 BaseIOSystemFactory.h \
						 ../config.h

CLEANFILES=core* tag cscope.* .*.sw*
DISTCLEANFILES=$(CLEANFILES)
