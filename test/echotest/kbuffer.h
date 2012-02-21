#ifndef __KBUFFER_H
#define __KBUFFER_H

typedef struct kbuffer_chunk {
	char *data;
	int size;
	int pt;
	struct kbuffer_chunk *prev;
	struct kbuffer_chunk *next;
}kbuffer_chunk;

typedef struct kbuffer {
	int size;
	kbuffer_chunk *head;
	kbuffer_chunk *tail;
}kbuffer;


kbuffer * kbuffer_new();
void kbuffer_free(kbuffer *buf);
void kbuffer_add_data(kbuffer *buf, const void* data, int size);
int kbuffer_get_size(kbuffer *buf);
const void * kbuffer_get_contiguous_data(kbuffer *buf, int *size);

#endif
