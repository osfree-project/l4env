#ifndef __LOCK_H__
#define __LOCK_H__

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <flux/c/stdlib.h>
#include <l4/log/l4log.h>

typedef struct wq_lock_queue_elem{
	volatile struct wq_lock_queue_elem*next,*prev;
	l4_threadid_t id;
	}wq_lock_queue_elem;
	
typedef struct{
	wq_lock_queue_elem *last;
	}wq_lock_queue_base;

/* Prototypes */
static inline int wq_lock_lock(wq_lock_queue_base*queue, wq_lock_queue_elem*q);
static inline int wq_lock_unlock(wq_lock_queue_base*queue, wq_lock_queue_elem*q);
static inline int wq_lock_locked(wq_lock_queue_base*queue);
//inline int wq_lock_try_lock(l4_simple_lock_t *lock);

/* Utilities */
static inline l4_umword_t i386_xchg(l4_umword_t *addr,l4_umword_t val){
  l4_umword_t tmp;
  __asm__ __volatile__ ("xchg	%0, (%2)\n\t"
			: "=r" (tmp)
			: "0" (val), "r" (addr)
			: "memory"
			);
  return tmp;
}

static inline l4_umword_t i386_cmpxchg(l4_umword_t *dest,l4_umword_t cmp, l4_umword_t val){
  l4_umword_t tmp;
    __asm__ __volatile__ ("cmpxchg %1, %3 \n\t"
                       : "=a" (tmp)	/* EAX, return val */
                       : "r" (val), "0" (cmp), "m" (*dest)
                       : "memory", "cc" );
  return tmp;
}

/* Implementation */
inline int wq_lock_lock(wq_lock_queue_base*queue, wq_lock_queue_elem*q){
  wq_lock_queue_elem *old;
  l4_msgdope_t result;
  int err;
  l4_umword_t dummy;
  
  q->next=NULL;
  q->id = l4_myself();
  old = (wq_lock_queue_elem*)i386_xchg((l4_umword_t*)(&queue->last),
                                          (l4_umword_t)q);
  if(old!=NULL){	/* already locked */
    //LOGL("locking %p, locked by %#t", queue, old->id);
    old->next = q;
    q->prev = old;
    if((err=l4_ipc_receive(old->id, L4_IPC_SHORT_MSG, &dummy, &dummy,
                                L4_IPC_NEVER, &result))!=0){
      //LOGl("error %#x receiving IPC from %#t", err, old->id);
      return err;
    }
    if((err= l4_ipc_send(old->id, L4_IPC_SHORT_MSG, 0,0,
                                L4_IPC_NEVER, &result))!=0){
      //LOGl("error %#x sending IPC to %#t", err, old->id);
      return err;
    }
    //LOGL("got lock");
  }
  return 0;
}

inline int wq_lock_unlock(wq_lock_queue_base*queue, wq_lock_queue_elem*q){
  wq_lock_queue_elem *other;
  l4_msgdope_t result;
  int err;
  l4_umword_t dummy;

  other = (wq_lock_queue_elem*)i386_cmpxchg((l4_umword_t*)(&queue->last), 
                                             (l4_umword_t)q,
                                             (l4_umword_t)NULL);
  if(other == q){	/* nobody wants the lock */
  }else{		/* someone wants the lock */
    //LOGL("unlocking %#t...", other->id);
    while(q->next != other){	/* 2 possibilities:
    				   - other is next, but didnt sign, give
    				     it the time
    				   - other is not next, find the next by
    				     backward iteration */
      if( other->prev == NULL){	/* - other didnt sign its prev, give it the
      				     time to do this */
      	l4_thread_switch(other->id);
      }else if (other->prev!=q){/* 2 poss:
      				    - if other is next it might be signed up
      				      to now (other->prev == q)
      				    - other is not something else then next
      				      (its not NULL, we know this), go
      				      backward */
        (volatile wq_lock_queue_elem*)other = other->prev;
      }
    }	/* while(q->next!=other */
    /* now we have the next in other */
    /* send an ipc, timeout never */
    if((err = l4_ipc_call(q->next->id, 
                               L4_IPC_SHORT_MSG, 0,0,
                               L4_IPC_SHORT_MSG, &dummy, &dummy,
                               L4_IPC_NEVER,
                               &result))!=0)return err;
  }	/* someone wants the lock */
  return 0;
}

inline int wq_lock_locked(wq_lock_queue_base*queue){
  return queue->last!=NULL;
}
#endif
