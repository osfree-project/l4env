/*
 * $Id$
 */

/*****************************************************************************
 * wait_queue.h                                                              *
 * wait queue defines                                                        *
 *****************************************************************************/
#ifndef __L4UTIL_WAIT_QUEUE_H
#define __L4UTIL_WAIT_QUEUE_H

/* L4 includes */
#include <l4/sys/types.h>

#ifndef NULL
#define NULL 0
#endif

/* wait queue */
struct l4_wait_queue_entry
{
  l4_threadid_t thread; 
  struct l4_wait_queue_entry *next;
};

typedef struct l4_wait_queue_entry l4_wait_queue_entry_t;

struct l4_wait_queue
{
  struct l4_wait_queue_entry *head;
  struct l4_wait_queue_entry *tail;
  l4_threadid_t owner;
};

typedef struct l4_wait_queue l4_wait_queue_t;

#define L4_WAIT_QUEUE_INITIALIZER {NULL,NULL,L4_INVALID_ID}
#define L4_WAIT_QUEUE_INIT ((l4_wait_queue_t)L4_WAIT_QUEUE_INITIALIZER)

/* prototypes */

inline void l4_wq_lock(l4_wait_queue_t *wq);
inline void l4_wq_unlock(l4_wait_queue_t *wq);
inline void l4_wq_remove_myself(l4_wait_queue_t *wq);

#endif /* __L4UTIL_WAIT_QUEUE_H */
