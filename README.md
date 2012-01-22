# Coconut
C++ Simple & High Performance Network Framework Using libevent2

## TCP Echo Server Example
#include "Coconut.h"
#include "NetworkHelper.h"
#include "IOServiceContainer.h"

class MyClientController : public coconut::BinaryController {
    virtual void onReceivedData(const void *data, int size) {
        socket()->write(data, size);
    }
};

class MyServerController : public coconut::ServerController {
    virtual boost::shared_ptr<coconut::ClientController> onAccept(boost::shared_ptr<coconut::TcpSocket> socket) {
        boost::shared_ptr<MyClientController> newController(new MyClientController);
        return newController;
    }
};

int main(int argc, char **argv) {
    coconut::IOServiceContainer ioServiceContainer(threadCount);
    ioServiceContainer.initialize();

    try {
        boost::shared_ptr<MyServerController> serverController(new MyServerController);
        coconut::NetworkHelper::listenTcp(&ioServiceContainer, 8000, serverController);
        ioServiceContainer.run();
    } catch(coconut::Exception &e) {
        // Error
    }
    return 0;
}


## Requirement

### hiredis
link : git://github.com/antirez/hiredis.git
on Win32, you must use 3rdParty/hiredis instead upper link so that the build is successful.
	  the hiredis in upper git link is not compiled on Win32.
	
### libevent 2.0.15 
link : https://github.com/downloads/libevent/libevent/libevent-2.0.16-stable.tar.gz

### boost 1.33.0 
link : http://sourceforge.net/projects/boost/files/boost/1.48.0/boost_1_48_0.tar.gz/download

### libjson 7.4.1
link : http://downloads.sourceforge.net/project/libjson/libjson_7.4.1.zip?r=http%3A%2F%2Fsourceforge.net%2Fprojects%2Flibjson%2F&ts=1327071561&use_mirror=cdnetworks-kr-2
in order to build successful on this frame work, you must use 3rdParty/libjson instead upper link's libjson.
	  We modified JSONStream class to parse correctly. (7.4.1 libjson has a few bug in JSONStream)


## Installing/Configuring

export BOOST_ROOT=<BOOST ROOT DIR>
export LD_LIBRARY_PATH=<BOOST STAGE LIB DIR>:$LD_LIBRARY_PATH

configure
make
cd tester
unittest

