lib_LTLIBRARIES = libcoconut.la

AM_CXXFLAGS = -Wall

AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(srcdir)/

libcoconut_la_SOURCES = \
                       dep/kbuffer.c \
                       Coconut.cpp \
                       Logger.cpp \
                       IOService.cpp \
                       IOServiceContainer.cpp \
					   DeferredCaller.cpp \
                       Timer.cpp \
                       DNSResolver.cpp \
                       BufferedTransport.cpp \
                       TcpSocket.cpp \
                       UdpSocket.cpp \
                       ConnectionListener.cpp \
                       RedisRequest.cpp \
                       RedisResponse.cpp \
                       HttpRequest.cpp \
                       BaseProtocol.cpp \
                       LineProtocol.cpp \
                       FrameProtocol.cpp \
                       FileDescriptorProtocol.cpp \
                       BaseController.cpp \
                       BaseControllerEvent.cpp \
                       ClientController.cpp \
                       ServerController.cpp  \
                       LineController.cpp \
                       FrameController.cpp \
                       FileDescriptorController.cpp \
                       HttpRequestController.cpp \
                       RedisController.cpp \
                       NetworkHelper.cpp

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
                         HttpRequest.h \
						 LineProtocol.h \
						 FrameProtocol.h \
                         FileDescriptorProtocol.h \
                         ClientController.h \
                         LineController.h \
                         FrameController.h \
                         FileDescriptorController.h \
                         ServerController.h \
                         RedisController.h \
                         HttpRequestController.h \
                         NetworkHelper.h

CLEANFILES=core* tag cscope.* .*.sw*
DISTCLEANFILES=$(CLEANFILES)
