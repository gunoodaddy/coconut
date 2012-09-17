/*
* Copyright (c) 2012, Eunhyuk Kim(gunoodaddy) 
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*   * Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*   * Neither the name of Redis nor the names of its contributors may be used
*     to endorse or promote products derived from this software without
*     specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __KBUFFER_H
#define __KBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

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
int kbuffer_add(kbuffer *buf, const void* data, int size);
void kbuffer_add_printf(kbuffer *buf, const char *format, ...);
int kbuffer_get_size(kbuffer *buf);
const void * kbuffer_get_contiguous_data(kbuffer *buf, int *size);
void kbuffer_drain(kbuffer *buf, int size);
int kbuffer_get_chunk_count(kbuffer *buf);

#ifdef  __cplusplus
}
#endif

#endif
