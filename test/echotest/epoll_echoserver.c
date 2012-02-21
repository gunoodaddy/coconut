#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "kbuffer.h"

#define NOTUSED(V) 			((void ) V)
#define IOBUF_LEN         	(1024*16)
#define MAX_EVENT_CNT		10240

void oom(const char *msg, ...) {
	char log[4096] = {0, };
	va_list args;
	va_start(args, msg);
	vsprintf(log, msg, args);
	va_end(args);

	printf("%s\n", log);
	sleep(1);
	abort();
}

//===========================================================================
// epoll definitions
//===========================================================================
struct epoller;

typedef void eventProc(struct epoller *epoll, int fd, void *privdata, int mask);

typedef struct epollEvent {
    int mask; 
    eventProc *readProc;
    eventProc *writeProc;
    void *privdata;
} epollEvent;

typedef struct epollFiredEvent {
	int fd;     
	int mask; 
} epollFiredEvent; 

typedef struct epoller {
	int stop;
	int epfd;
	int maxfd;
	struct epoll_event epdata[MAX_EVENT_CNT];
	epollFiredEvent fired[MAX_EVENT_CNT];
	epollEvent events[MAX_EVENT_CNT];
}epoller;

epoller *epollCreate() {
	epoller *poller = ( epoller *)malloc(sizeof(epoller));
	memset(poller, 0, sizeof(epoller));

	if((poller->epfd = epoll_create(1024)) < 0) {
		oom("not created epoll descriptor!");
	}

	int i;
    for (i = 0; i < MAX_EVENT_CNT; i++)
        poller->events[i].mask = 0;

	return poller;
}

void epollDelete(epoller *epoll) {
	close(epoll->epfd);
    free(epoll);
}

int epollAddEvent(epoller *epoll, int fd, int mask, eventProc *proc, void *privdata) {
	if (fd >= MAX_EVENT_CNT) {
		oom("epollAddEvent() : fd max limited");
		return -1;
	}

	epollEvent *ev = &epoll->events[fd];

	struct epoll_event ee;
	/* If the fd was already monitored for some event, we need a MOD
	 * operation. Otherwise we need an ADD operation. */
	int op = ev->mask == 0 ?  EPOLL_CTL_ADD : EPOLL_CTL_MOD;

	ee.events = 0;
	ee.events = mask | ev->mask; /* Merge old events */
	ee.data.u64 = 0; /* avoid valgrind warning */
	ee.data.fd = fd;
	//printf("epoll add %d : 0x%x\n", fd, mask);
	if (epoll_ctl(epoll->epfd, op, fd, &ee) == -1) {
		oom("epollAddEvent() : epoll_ctl error");
		return -1;
	}

	ev->mask |= mask;
	if (mask & EPOLLIN) ev->readProc = proc;
	if (mask & EPOLLOUT) ev->writeProc = proc;

	ev->privdata = privdata;
	if (fd > epoll->maxfd)
		epoll->maxfd = fd;
	return 0;
}

void epollDelEvent(epoller *epoll, int fd, int mask) {
	if (fd >= MAX_EVENT_CNT) {
		oom("epollDelEvent() : fd max limited");
		return;
	}

	epollEvent *ev = &epoll->events[fd];
	if (ev->mask == 0) {
		printf("event mask is zero : fd = %d\n", fd);
		return;
	}

	ev->mask = ev->mask & (~mask);
	if (fd == epoll->maxfd && ev->mask == 0) {
		/* Update the max fd */
		int j; 
		for (j = epoll->maxfd-1; j >= 0; j--)
			if (epoll->events[j].mask != 0) 
				break;
		epoll->maxfd = j;
	}

	struct epoll_event ee;
	int op = (ev->mask == 0) ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;
	ee.events = ev->mask;
	ee.data.u64 = 0; /* avoid valgrind warning */
	ee.data.fd = fd;

	/* Note, Kernel < 2.6.9 requires a non null event pointer even for
	 * EPOLL_CTL_DEL. */
	//printf("epoll delete %d : op = %d, mask = 0x%x\n", fd, op, ee.events);
	epoll_ctl(epoll->epfd, op, fd, &ee);
}

int epollPolling(epoller *epoll, struct timeval *tvp) {
	int retval, numevents = 0;
	retval = epoll_wait(epoll->epfd, epoll->epdata, MAX_EVENT_CNT,
			tvp ? (tvp->tv_sec*1000 + tvp->tv_usec/1000) : -1);

	if (retval > 0) {
		int i;
		numevents = retval;
		for (i = 0; i < numevents; i++) {
			struct epoll_event *e = epoll->epdata + i;
			epoll->fired[i].fd = e->data.fd;
			epoll->fired[i].mask = e->events;
		}
	}
	return numevents;
}

void epollDispatch(epoller *epoll) {
	epoll->stop = 0;
	while (!epoll->stop) {
		if (epoll->maxfd <= 0)
			break;

		int i;
		int processed = 0, numevents;
		numevents = epollPolling(epoll, NULL);

		for (i = 0; i < numevents; i++) {
			epollEvent *ev = &epoll->events[epoll->fired[i].fd];
			int mask = epoll->fired[i].mask;
			int fd = epoll->fired[i].fd;
			int rfired = 0;

			//  ev->mask & mask & ... code : for valid event check
			if (ev->mask & mask & EPOLLIN) {
				rfired = 1;

				ev->readProc(epoll,fd, ev->privdata, mask);
			}
			if (ev->mask & mask & EPOLLOUT) {
				if (!rfired || ev->writeProc != ev->readProc)
					ev->writeProc(epoll, fd, ev->privdata, mask);
			}
			processed++;
		}
	}
}

//===========================================================================
// socket util definition
//===========================================================================

int createSocket() {
	int s, on = 1;
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		return -1;
	}
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
		return -1;
	}
	return s;
}

//===========================================================================
// echo server definition
//===========================================================================

typedef struct echoServerContext {
	int ipfd;
	int serverfd;
	int port;
	epoller *epoll;
}echoServerContext;

typedef struct echoClientContext {
	int fd;
	kbuffer *writebuffer; 
}echoClientContext;


///////////////////////////////////////////////////////////////////////////////////////

echoServerContext *createTcpServer(epoller *epoll, int port) {
	int s;
	struct sockaddr_in sa;

	if ((s = createSocket()) == -1) {
		oom("socket creation error : %s", strerror(errno));
		return NULL;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(s, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
		close(s);
		oom("bind error : %s", strerror(errno));
		return NULL;
	}
	if (listen(s, 511) == -1) { /* the magic 511 constant is from nginx */
		close(s);
		oom("listen error : %s", strerror(errno));
		return NULL;
	} 

	echoServerContext *server = (echoServerContext *)malloc(sizeof(echoServerContext));
	memset(server, 0, sizeof(echoServerContext));
	
	server->port = port;
	server->epoll = epoll;
	server->serverfd = s;
	return server;
}


//--------------------------------------------------------------------------------
// CLIENT
void writeDataToClient(epoller *epoll, int fd, void *privdata, int mask) {
	echoClientContext *client = (echoClientContext *)privdata;

	int size;
	int destroy = 0;
	do {
		if(kbuffer_get_size(client->writebuffer) <= 0) {
			epollDelEvent(epoll, fd, EPOLLOUT);
			break;
		}

		const void *data = kbuffer_get_contiguous_data(client->writebuffer, &size);
		int nwrite = write(fd, data, size);
		if(nwrite > 0) {
			kbuffer_drain(client->writebuffer, nwrite);
		} else if(nwrite == 0) {
			destroy = 1;
			break;
		} else if(nwrite < 0) {
			if(errno == EAGAIN) {
				nwrite = 0;
			} else {
				destroy = 1;
			}
			break;
		}
	} while(0);

	if(destroy) {
		close(fd);
		epollDelEvent(epoll, fd, EPOLLIN | EPOLLOUT);
	}
}


void readDataFromClient(epoller *epoll, int fd, void *privdata, int mask) {
	echoClientContext *client = (echoClientContext *)privdata;

	NOTUSED(client);
	
	char buf[IOBUF_LEN];
	int nread;
	NOTUSED(epoll);
	NOTUSED(mask);

	nread = read(fd, buf, IOBUF_LEN);
	int destroy = 0;
	if(nread > 0) {
		kbuffer_add_data(client->writebuffer, buf, nread);
		if(epollAddEvent(epoll, fd, EPOLLOUT, writeDataToClient, client) != 0)
			destroy = 1;
	}
	else if(nread == 0) {
		destroy = 1;
	} else if(nread < 0) {
		if(errno == EAGAIN) {
			nread = 0;
		} else {
			destroy = 1;
		}
	}

	if(destroy) {
		close(fd);
		epollDelEvent(epoll, fd, EPOLLIN | EPOLLOUT);
	}
}

//--------------------------------------------------------------------------------
// SERVER
void acceptTcpHandler(epoller *epoll, int fd, void *privdata, int mask) {

	int cport, cfd = -1;
	char cip[128] = {0, };
	echoServerContext *server = (echoServerContext *)privdata;

	NOTUSED(server);
	NOTUSED(epoll);
	NOTUSED(mask);

	struct sockaddr_in sa;
	socklen_t salen = sizeof(sa);

	while(1) {
		cfd = accept(fd, (struct sockaddr *)&sa, &salen);
		if (cfd == -1) {
			if (errno == EINTR)
				continue;
			else {
				oom("accept error : %s", strerror(errno));
				return;
			}
		}
		break;
	}

	if (cfd == -1) {
		printf("failed to accepting client connection\n");
		return;
	}

	strcpy(cip, inet_ntoa(sa.sin_addr));
	cport = ntohs(sa.sin_port);

	printf("Accepted %s:%d\n", cip, cport);

	// client object create..
	echoClientContext *c = (echoClientContext *)malloc(sizeof(echoClientContext));
	c->writebuffer = kbuffer_new();
	if (epollAddEvent(epoll, cfd, EPOLLIN, readDataFromClient, c) != 0)
		oom("failed to add client event..");
}

int main(int argc, char **argv) {

	if(argc < 2) {
		printf("usage : %s <port>\n", argv[0]);
		exit(1);
	}

	int port = atoi(argv[1]);

	epoller *ep = epollCreate();

	echoServerContext *server = createTcpServer(ep, port);
	
	if(epollAddEvent(ep, server->serverfd, EPOLLIN, acceptTcpHandler, NULL) != 0)
		oom("creating file event");

	epollDispatch(ep);

	epollDelete(ep);
	printf("Bye!\n");
	return 0;
}
