# Coconut
Coconut is C++ network framework with briefly, clearly, high performance.

* Using libevent (available for switching another network library)
* Available cross platform linux, windows (by using libevent)
* Tcp Socket
* Unix Domain Socket(only tcp)
* Udp Socket
* Http Client (GET, POST)
* Simple Http Server
* Redis Request (optional)
* JSON Stream Protocol (optional, now unstable)

## TCP Echo Server Example

total 26 lines.
(It's not iocp sample.)

    #include "Coconut.h"
    using namespace coconut;

    class FooClient : public BinaryController {
        virtual void onReceivedData( const void *data, int size ) {
            socket()->write(data, size);
        }
    };
    class FooServer : public ServerController {
        virtual boost::shared_ptr<ClientController> onAccept( boost::shared_ptr<TcpSocket> socket ) {
            boost::shared_ptr<FooClient> newClient( new FooClient );
            return newClient;
        }
    };
    int main( int argc, char **argv ) {
        IOServiceContainer container( 16 /* thread count */ );
        container.initialize();
        try {
            boost::shared_ptr<FooServer> fooServer( new FooServer );
            NetworkHelper::listenTcp( &container, 8000, fooServer );
            container.run();
        } catch(Exception &e) {
            // Error
        }
        return 0;
    }

## TCP Echo Server Performance Result 

<img src="https://github.com/gunoodaddy/coconut/raw/master/test/echotest/tps_result.png"/>

<b>Environment</b>

* Server : single thread, Linux, Intel(R) Xeon(R) CPU L5520  @ 2.27GHz 16 cpu core
* Client : 16 thread, 1000 clients, 10000 count per a client with 256 byte, Linux, Intel(R) Xeon(R) CPU L5520  @ 2.27GHz 12 cpu core

* All server sample codes are here : https://github.com/gunoodaddy/coconut/tree/master/test/echotest
* client code are here : https://github.com/gunoodaddy/coconut/blob/master/examples/tcpclient.cpp

## Requirement

### libevent >= 2.0.15
http://libevent.org/

* If you use Win32 IOCP, you must use this [libevent](https://github.com/gunoodaddy/Libevent).
* Original libevent >= 2.0.15 has a bug in `evutil_tv_to_msec` function. (need to add *tv* pointer null check code) 
* And we've added `free callback feature` for struct evhttp_request free callback feature to know which evhttp_request pointer will be freed. It needs if you should store http request and then later send responses. (like 'Commet'). You can see our fixed codes by finding `gunoodaddy` comment in this [libevent](https://github.com/gunoodaddy/Libevent) sources.
* By running `configure`, it will check our `free callback feature` exists in your own libevent and then mark `HAVE_LIBEVENT_GUNOODADDY_FIX` in config.h.
	
### boost >= 1.33.0 
[boost 1.48.0 download](http://sourceforge.net/projects/boost/files/boost/1.48.0/boost_1_48_0.tar.gz/download)

* You must build boost library (run b2), we need boost stage library for using boost::thread

### hiredis (option : --without-redis)
https://github.com/antirez/hiredis

* On Win32, you must use this [hiredis](https://github.com/gunoodaddy/hiredis) instead of orginal hiredis so that the build is successful.
* We modified `redisFormatCommandArgv` function in koenvandesande's sources.
* In win32, sprintf `%zu` format is not valid. so changed `%d` if Win32   
* If you do not want to use redis, run configure `--without-redis`.
  

### libjson 7.4.1 (option : --without-json)
[libjson 7.4.1 download](http://downloads.sourceforge.net/project/libjson/libjson_7.4.1.zip?r=http%3A%2F%2Fsourceforge.net%2Fprojects%2Flibjson%2F&ts=1327071561&use_mirror=cdnetworks-kr-2)
    
* In order to build successful, you must use `3rdParty/src/libjson` instead upper link's libjson.
* We modified JSONStream class to parse correctly. (7.4.1 libjson has a few bug in JSONStream)
* You can find 'gunoodaddy' comment in `3rdParty/src/libjson` modified codes.
* And if you use linux 64bit os, must edit \<lib_json_root\>/makefile to add `-fPIC` to cxxflags_default.
* If you do not want to use json feature, run configure `--without-json`.

## Build / Installation 

    export BOOST_ROOT=<BOOST ROOT DIR>
    export LD_LIBRARY_PATH=<BOOST STAGE LIB DIR>:$LD_LIBRARY_PATH

    configure
    make
    cd tester
    unittest

## TODO before linux configuration

	autoheader
	(optional) cp  /usr/share/libtool/config/ltmain.sh .
	automake -a --copy
	autoconf

