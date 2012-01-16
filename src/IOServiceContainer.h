#pragma once

#include <vector>
#include <signal.h>
#include "BaseIOServiceContainer.h"

namespace coconut {

class COCONUT_API IOServiceContainer : public BaseIOServiceContainer {
public:
	IOServiceContainer(int threadCount = 0) : threadCount_(threadCount) 
#if defined(WIN32)
		, iocpEnabled_(false)
		, cpuCnt_(0)
#endif
	{
	}

	~IOServiceContainer();

public:
	boost::shared_ptr<IOService> ioService();
	void initialize();
	void run();
	void stop();
#if defined(WIN32)
	void turnOnIOCP(size_t cpuCount);	// must call before initialize()
#endif

private:
	int threadCount_;
#if defined(WIN32)
	bool iocpEnabled_;
	size_t cpuCnt_;
#endif
	std::vector< boost::shared_ptr<IOService> > ioservices_;
};

}
