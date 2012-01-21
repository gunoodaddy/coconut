#pragma once

#if ! defined(COCONUT_USE_PRECOMPILE)
#include <boost/shared_ptr.hpp>
#endif
#include "BaseIOServiceContainer.h"
#include "ServerController.h"
#include "ClientController.h"
#include "HttpClientController.h"
#include "RedisController.h"

namespace coconut {

class COCONUT_API NetworkHelper {
public:
	static void connectTcp( BaseIOServiceContainer *ioServiceContainer,
                            const char* host, 
                            int port, boost::shared_ptr<ClientController> controller, 
                            int timeout = 0);

	static void connectUnix( BaseIOServiceContainer *ioServiceContainer,
                             const char* path, 
                             boost::shared_ptr<ClientController> controller, 
                             int timeout = 0);

	static void listenTcp( BaseIOServiceContainer *ioServiceContainer,
                           int port, 
                           boost::shared_ptr<ServerController> controller);

	static void listenUnix( BaseIOServiceContainer *ioServiceContainer,
                            const char *path, 
                            boost::shared_ptr<ServerController> controller);

	static void bindUdp( BaseIOServiceContainer *ioServiceContainer, 
                         int port, 
                         boost::shared_ptr<ClientController> controller);

	static void connectRedis( BaseIOServiceContainer *ioServiceContainer, 
                              const char* host, 
                              int port, 
                              boost::shared_ptr<RedisController> controller);
};

}
