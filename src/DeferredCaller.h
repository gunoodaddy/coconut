#pragma once

#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#endif
namespace coconut {

class IOService;
class DeferredCallerImpl;

typedef boost::function< void () > deferedMethod_t;

class COCONUT_API DeferredCaller {
public:
	DeferredCaller();
	DeferredCaller(boost::shared_ptr<IOService> ioService); 
	~DeferredCaller();

public:
	void setIOService(boost::shared_ptr<IOService> ioService);
	void deferredCall(deferedMethod_t func);

private:
	DeferredCallerImpl *impl_;
};

}
