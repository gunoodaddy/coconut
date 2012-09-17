#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "kbuffer.h"

kbuffer * kbuffer_new() {
	kbuffer *buffer = (kbuffer*) malloc(sizeof(kbuffer));
	memset(buffer, 0, sizeof(kbuffer));
	return buffer;
}

kbuffer_chunk *_kbuffer_chuck_free(kbuffer *buf, kbuffer_chunk *chunk) {
	kbuffer_chunk *prev = chunk->prev;
	kbuffer_chunk *next = chunk->next;
	if(prev) {
		prev->next = next;
	}
	if(next) {
		next->prev = prev;
	}
	if(buf->tail == chunk) {
		buf->tail = prev;
	}
	if(buf->head == chunk) {
		buf->head = next;
	}
	buf->size -= (chunk->size - chunk->pt);
	free(chunk->data);
	free(chunk);
	return next;
}

void _kbuffer_chuck_remove(kbuffer *buf, kbuffer_chunk *chunk, int size) {
	buf->size -= size;
	chunk->pt += size;
}

void kbuffer_free(kbuffer *buf) {
	kbuffer_chunk *next = buf->head;
	while(next) {
		next = _kbuffer_chuck_free(buf, next);
	}
	free(buf);
}

int kbuffer_add(kbuffer *buf, const void* data, int size) {
	kbuffer_chunk *chunk = (kbuffer_chunk *)malloc(sizeof(kbuffer_chunk));
	memset(chunk, 0, sizeof(kbuffer_chunk)); // very important code..
	chunk->data = (char *)malloc(size);
	chunk->size = size;
	chunk->prev = buf->tail;
	chunk->next = NULL;
	memcpy(chunk->data, data, size);

	if(buf->tail) {
		buf->tail->next = chunk;
	}
	buf->tail = chunk;
	if(NULL == buf->head)
		buf->head = chunk;
	buf->size += size;
	//printf("kbuffer_add :: %d(%d) / this = %p, chunk = %p, head = %p, tail = %p\n",  size, buf->size, buf, chunk, buf->head, buf->tail);
	return size;
}

void kbuffer_add_printf(kbuffer *buf, const char *format, ...) {
	char log[4000];
	va_list args;
	va_start(args, format);
	vsnprintf(log, sizeof(log)-1, format, args);
	va_end(args);

	kbuffer_add(buf, log, strlen(log));
}

int kbuffer_get_size(kbuffer *buf) {
	return buf->size;
}

const void * kbuffer_get_contiguous_data(kbuffer *buf, int *size) {
	if(buf->head) {
		if(size) *size = buf->head->size - buf->head->pt;
		return buf->head->data + buf->head->pt;
	} else {
	}
	if(size) *size = 0;
	
	return NULL;
}

int kbuffer_get_chunk_count(kbuffer *buf) {
	int c = 0;
	kbuffer_chunk *next = buf->head;
	while(next) {
		next = next->next;
		c++;
	}
	return c;
}

void kbuffer_drain(kbuffer *buf, int size) {
	kbuffer_chunk *next = buf->head;
	while(next) {
		int cs = next->size - next->pt;
		if(cs <= size) {
			size -= cs;
			next = _kbuffer_chuck_free(buf, next);
			if(size == 0)
				break;
		} else {
			_kbuffer_chuck_remove(buf, next, size);
			break;
		}
	}
}


