# Coconut
C++ Simple & High Performance Network Framework Using libevent2

* TcpSocket
* UdpSocket
* Redis Request
* Http Client (GET, POST)
* Simple Http Server
* JSON Stream Protocol

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
https://github.com/antirez/hiredis

* On Win32, you must use 3rdParty/hiredis instead upper link so that the build is successful.
* The hiredis in upper git link is not compiled on Win32.
* or use this git repository https://github.com/koenvandesande/hiredis.git
	
### libevent 2.0.15 
https://github.com/downloads/libevent/libevent/libevent-2.0.16-stable.tar.gz

* If you use Win32 IOCP, you must use 3rdParty/libevent.tgz.
  Original libevent >= 2.0.15 has a bug in evutil_tv_to_msec function. (need to include tv pointer null check code) 

### boost 1.33.0 
http://sourceforge.net/projects/boost/files/boost/1.48.0/boost_1_48_0.tar.gz/download

* You must build boost library (run b2), we need boost stage library for using boost::thread

### libjson 7.4.1
http://downloads.sourceforge.net/project/libjson/libjson_7.4.1.zip?r=http%3A%2F%2Fsourceforge.net%2Fprojects%2Flibjson%2F&ts=1327071561&use_mirror=cdnetworks-kr-2
    
* In order to build successful on this frame work, you must use 3rdParty/libjson instead upper link's libjson.
* We modified JSONStream class to parse correctly. (7.4.1 libjson has a few bug in JSONStream)


## Installing/Configuring

    export BOOST_ROOT=<BOOST ROOT DIR>
    export LD_LIBRARY_PATH=<BOOST STAGE LIB DIR>:$LD_LIBRARY_PATH

    configure
    make
    cd tester
    unittest

