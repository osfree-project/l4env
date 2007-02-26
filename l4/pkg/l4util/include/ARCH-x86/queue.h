#ifndef __L4UTIL_QUEUE_H__ 
#define __L4UTIL_QUEUE_H__ 

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>

struct l4_buffer_head
{
  l4_threadid_t src;
  l4_uint32_t len;
  char *buffer;
};

int l4_queue_dequeue(struct l4_buffer_head **buffer);

int l4_queue_init(int queue_threadno,
		  void *(*malloc_func)(l4_uint32_t size),
		  l4_umword_t max_rcv);

#endif /* !__L4UTIL_QUEUE_H__ */
