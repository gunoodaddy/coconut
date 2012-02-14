#pragma once
#include <boost/pool/object_pool.hpp>
#include "BufferedTransport.h"
#include "FrameProtocol.h"
#include "JSONProtocol.h"
#include "LineProtocol.h"
#include "FileDescriptorProtocol.h"
#include "LineController.h"
#include "JSONController.h"
#include "FrameController.h"
#include "FileDescriptorController.h"

namespace coconut { namespace default_allocator {

	using namespace protocol;

#define DECLARE_DEFAULT_OBJECT_ALLOCATOR(clazz)	\
	class clazz##Allocator : public BaseObjectAllocator<clazz>::Allocator\
	{	\
		clazz* make() {\
			ScopedMutexLock(lock_);\
			clazz *p = pool_.construct();\
			_LOG_TRACE("OBJECT POOL : CREATE "#clazz"() : %p\n", p);\
			return p;\
		}\
		void destroy(clazz *p) {\
			ScopedMutexLock(lock_);\
			_LOG_TRACE("OBJECT POOL : DESTROY ~"#clazz"() : %p\n", p);\
			pool_.destroy(p);\
		}\
		boost::shared_ptr<clazz> makeSharedPtr() {\
			return boost::shared_ptr<clazz>(\
						make(), BaseObjectAllocator<clazz>::destroyFunction());\
		}\
		static boost::object_pool<clazz> pool_;\
	public:\
		Mutex lock_;\
	};\
	boost::object_pool<clazz> clazz##Allocator::pool_;


#define INSTALL_DEFAULT_ALLOCATOR(clazz) \
	clazz::setAllocator(boost::shared_ptr<clazz##Allocator>(new clazz##Allocator));

	DECLARE_DEFAULT_OBJECT_ALLOCATOR(DeferredCaller);
	DECLARE_DEFAULT_OBJECT_ALLOCATOR(DNSResolver);
	DECLARE_DEFAULT_OBJECT_ALLOCATOR(ConnectionListener);
	DECLARE_DEFAULT_OBJECT_ALLOCATOR(BufferedTransport);
	DECLARE_DEFAULT_OBJECT_ALLOCATOR(JSONProtocol);
	DECLARE_DEFAULT_OBJECT_ALLOCATOR(LineProtocol);
	DECLARE_DEFAULT_OBJECT_ALLOCATOR(FileDescriptorProtocol);
	DECLARE_DEFAULT_OBJECT_ALLOCATOR(FrameProtocol);
	DECLARE_DEFAULT_OBJECT_ALLOCATOR(LibeventConnectionListenerImpl);
	DECLARE_DEFAULT_OBJECT_ALLOCATOR(LibeventTimerImpl);
	DECLARE_DEFAULT_OBJECT_ALLOCATOR(LibeventDeferredCallerImpl);
	DECLARE_DEFAULT_OBJECT_ALLOCATOR(LibeventUdpSocketImpl);
	DECLARE_DEFAULT_OBJECT_ALLOCATOR(LibeventTcpSocketImpl);
	DECLARE_DEFAULT_OBJECT_ALLOCATOR(LibeventIOServiceImpl);
	DECLARE_DEFAULT_OBJECT_ALLOCATOR(LibeventDNSResolverImpl);
	DECLARE_DEFAULT_OBJECT_ALLOCATOR(LibeventHttpServerImpl);
	DECLARE_DEFAULT_OBJECT_ALLOCATOR(LibeventHttpRequestImpl);
	DECLARE_DEFAULT_OBJECT_ALLOCATOR(LibeventHttpClientImpl);

	void doInstall() {
		INSTALL_DEFAULT_ALLOCATOR(DeferredCaller);
		INSTALL_DEFAULT_ALLOCATOR(DNSResolver);
		INSTALL_DEFAULT_ALLOCATOR(ConnectionListener);
		INSTALL_DEFAULT_ALLOCATOR(BufferedTransport);
		INSTALL_DEFAULT_ALLOCATOR(JSONProtocol);
		INSTALL_DEFAULT_ALLOCATOR(LineProtocol);
		INSTALL_DEFAULT_ALLOCATOR(FileDescriptorProtocol);
		INSTALL_DEFAULT_ALLOCATOR(FrameProtocol);
		INSTALL_DEFAULT_ALLOCATOR(LibeventConnectionListenerImpl);
		INSTALL_DEFAULT_ALLOCATOR(LibeventTimerImpl);
		INSTALL_DEFAULT_ALLOCATOR(LibeventDeferredCallerImpl);
		INSTALL_DEFAULT_ALLOCATOR(LibeventUdpSocketImpl);
		INSTALL_DEFAULT_ALLOCATOR(LibeventTcpSocketImpl);
		INSTALL_DEFAULT_ALLOCATOR(LibeventIOServiceImpl);
		INSTALL_DEFAULT_ALLOCATOR(LibeventDNSResolverImpl);
		INSTALL_DEFAULT_ALLOCATOR(LibeventHttpServerImpl);
		INSTALL_DEFAULT_ALLOCATOR(LibeventHttpRequestImpl);
		INSTALL_DEFAULT_ALLOCATOR(LibeventHttpClientImpl);
	}
}}
