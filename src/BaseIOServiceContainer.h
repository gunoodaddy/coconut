#pragma once

#include "config.h"
#include <signal.h>
#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#endif

namespace coconut {

class IOService;

class COCONUT_API BaseIOServiceContainer {
public:
	BaseIOServiceContainer() {
#if ! defined(WIN32)
    	signal(SIGPIPE, SIG_IGN);
#endif
	}

	virtual ~BaseIOServiceContainer() { }
	virtual void initialize() = 0;
	virtual void run() = 0;
	virtual void stop() = 0;

	virtual boost::shared_ptr<IOService> ioService() = 0;
};

}

