# Coconut
C++ Simple & High Performance Network Opensource Framework Using libevent2

* Available cross platform linux, win32
* Tcp Socket
* Unix Domain Socket(only tcp)
* Udp Socket
* Redis Request
* Http Client (GET, POST)
* Simple Http Server
* JSON Stream Protocol

## TCP Echo Server Example

total 30 lines.

    #include "Coconut.h"
    #include "NetworkHelper.h"
    #include "IOServiceContainer.h"
    using namespace coconut;

    class FooClient : public BinaryController {
        virtual void onReceivedData( const void *data, int size ) {
            socket()->write(data, size);
        }
    };
    class FooServer : public ServerController {
        virtual boost::shared_ptr<ClientController> onAccept( boost::shared_ptr<TcpSocket> socket ) {
            boost::shared_ptr<FooClient> newController(new FooClient);
            return newController;
        }
    };
    int main(int argc, char **argv) {
        IOServiceContainer container(threadCount);
        container.initialize();

        try {
            boost::shared_ptr<FooServer> fooServer(new FooServer);
            NetworkHelper::listenTcp(&container, 8000, fooServer);
            container.run();
        } catch(Exception &e) {
            // Error
        }
        return 0;
    }

## Requirement

### libevent 2.0.15 
https://github.com/downloads/libevent/libevent/libevent-2.0.16-stable.tar.gz

* If you use Win32 IOCP, you must use `3rdParty/src/libevent.tgz`.
  Original libevent >= 2.0.15 has a bug in `evutil_tv_to_msec` function. (need to add *tv* pointer null check code) 
* Or you should fix this bug in your own libevent source.
	
### boost 1.33.0 
http://sourceforge.net/projects/boost/files/boost/1.48.0/boost_1_48_0.tar.gz/download

* You must build boost library (run b2), we need boost stage library for using boost::thread

### hiredis
https://github.com/antirez/hiredis

* On Win32, you must use `3rdParty/src/hiredis` instead upper link so that the build is successful.
  The hiredis in upper git link is not compiled on Win32.
* Or use this git repository https://github.com/koenvandesande/hiredis.git

### libjson 7.4.1
http://downloads.sourceforge.net/project/libjson/libjson_7.4.1.zip?r=http%3A%2F%2Fsourceforge.net%2Fprojects%2Flibjson%2F&ts=1327071561&use_mirror=cdnetworks-kr-2
    
* In order to build successful on this frame work, you must use `3rdParty/src/libjson` instead upper link's libjson.
* We modified JSONStream class to parse correctly. (7.4.1 libjson has a few bug in JSONStream)
* You can find 'gunoodaddy' comment in `3rdParty/src/libjson` modified codes.

## Installing/Configuring

    export BOOST_ROOT=<BOOST ROOT DIR>
    export LD_LIBRARY_PATH=<BOOST STAGE LIB DIR>:$LD_LIBRARY_PATH

    configure
    make
    cd tester
    unittest

