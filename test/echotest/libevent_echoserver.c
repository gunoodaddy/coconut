
#include <event2/event.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "kbuffer.h"

#define IOBUF_LEN         (1024*16)

typedef struct echoClient {
	struct event *ev_read;
	struct event *ev_write;
	kbuffer *writebuffer;
}echoClient;


static void write_cb(evutil_socket_t fd, short what, void *arg) {
	echoClient *client = (echoClient *)arg;

	int size;
	int destroy = 0;
	do {
		if(kbuffer_get_size(client->writebuffer) <= 0) {
			event_del(client->ev_write);
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
		event_del(client->ev_read);
		event_del(client->ev_write);
	}

}


static void read_cb(evutil_socket_t fd, short what, void *arg) {
	echoClient *client = (echoClient *)arg;
	char buf[IOBUF_LEN];
	int nread;
	nread = read(fd, buf, IOBUF_LEN);
	int destroy = 0;
	if(nread > 0) {
		kbuffer_add_data(client->writebuffer, buf, nread);
		event_add(client->ev_write, NULL);
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
		event_del(client->ev_read);
		event_del(client->ev_write);
	}
}


void do_accept(evutil_socket_t listener, short event, void *arg)
{
	struct event_base *base = arg;
	struct sockaddr_storage ss;
	socklen_t slen = sizeof(ss);
	int fd = accept(listener, (struct sockaddr*)&ss, &slen);
	if (fd < 0) {
		perror("accept");
	} else if (fd > FD_SETSIZE) {
		close(fd);
	} else {
		printf("Accept!\n");
		evutil_make_socket_nonblocking(fd);
		echoClient *client = (echoClient *)malloc(sizeof(echoClient));
		client->writebuffer = kbuffer_new();
		client->ev_read = event_new(base, fd, EV_READ|EV_PERSIST, read_cb, (void*)client);
		client->ev_write = event_new(base, fd, EV_WRITE|EV_PERSIST, write_cb, (void*)client);

		event_add(client->ev_read, NULL);
	}
}



int
main(int argc, char **argv)
{
	evutil_socket_t listener;
	struct sockaddr_in sin;
	struct event_base *base;
	struct event *listener_event;

	base = event_base_new();
	if (!base)
		return; /*XXXerr*/

	int port = 9876;
	if (argc > 1) {
		port = atoi(argv[1]);
	}
	if (port<=0 || port>65535) {
		puts("Invalid port");
		return 1;
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	sin.sin_port = htons(port);

	listener = socket(AF_INET, SOCK_STREAM, 0);
	evutil_make_socket_nonblocking(listener);

	{
		int one = 1;
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	}

	if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
		perror("bind");
		return;
	}

	if (listen(listener, 16)<0) {
		perror("listen");
		return;
	}

	listener_event = event_new(base, listener, EV_READ|EV_PERSIST, do_accept, (void*)base);
	/*XXX check it */
	event_add(listener_event, NULL);

	event_base_dispatch(base);
	return 0;
}
