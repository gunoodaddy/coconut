CXX=g++
BOOST_DIR=/home/gtalk/src/boost_1_47_0
CXXFLAGS=-L/usr/local/lib -L${BOOST_DIR}/stage/lib -I${BOOST_DIR} 

TARGETS=epoll_echoserver libevent_echoserver asio_echoserver
all:${TARGETS}

epoll_echoserver: epoll_echoserver.o kbuffer.o
	$(CXX) -o epoll_echoserver epoll_echoserver.o kbuffer.o $(CXXFLAGS) 

libevent_echoserver: libevent_echoserver.o kbuffer.o
	$(CXX) -o libevent_echoserver libevent_echoserver.o kbuffer.o $(CXXFLAGS) -levent

asio_echoserver: asio_echoserver.o
	$(CXX) -o asio_echoserver asio_echoserver.o $(CXXFLAGS) -lboost_system

clean:
	rm -f *.o ${TARGETS}
