CXX=g++

CFLAGS=-L/usr/local/lib -I$(LIBUV_PATH)/include 
CXXFLAGS=-L/usr/local/lib -L${BOOST_ROOT}/stage/lib -I${BOOST_ROOT}

TARGETS=epoll_echoserver libevent_echoserver libuv_echoserver asio_echoserver
all:${TARGETS}

epoll_echoserver: epoll_echoserver.o kbuffer.o
	$(CXX) -o epoll_echoserver epoll_echoserver.o kbuffer.o $(CFLAGS) 

libevent_echoserver: libevent_echoserver.o kbuffer.o
	$(CXX) -o libevent_echoserver libevent_echoserver.o kbuffer.o $(CFLAGS) -levent

libuv_echoserver: libuv_echoserver.o
	$(CXX) -o libuv_echoserver libuv_echoserver.o $(CFLAGS) -I$(LIBUV_PATH)/include ${LIBUV_PATH}/uv.a -lpthread -lrt -lm

asio_echoserver: asio_echoserver.o
	$(CXX) -o asio_echoserver asio_echoserver.o $(CXXFLAGS) -lboost_system

clean:
	rm -f *.o ${TARGETS}
