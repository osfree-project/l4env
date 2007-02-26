/*
 * $Id$
 */

/*****************************************************************************
 * wait_queue.c                                                              *
 * simple wait queue implementation                                          *
 *****************************************************************************/


/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>

#include <l4/util/wait_queue.h>

#define __sti() __asm__ __volatile__ ("sti": : :"memory")
#define __cli() __asm__ __volatile__ ("cli": : :"memory")

#define __save_flags(x) \
__asm__ __volatile__("pushfl ; popl %0":"=g" (x): /* no input */ :"memory")

#define __restore_flags(x) \
__asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"g" (x):"memory")

#define WQ_WAKEUP_MAGIC   0xfee1dead
#define WQ_WAKEUP_TIMEOUT L4_IPC_TIMEOUT(98,9,0,0,0,0)  /* 100ms */

/* wakup message */
struct l4_wq_wakeup_msg
{
  l4_umword_t   rcv_fpage;    /* receive fpage */
  l4_msgdope_t  size;         /* size dope */
  l4_msgdope_t  send;         /* send dope */

  l4_threadid_t new_wait;     /* new wakeup thread */
  l4_umword_t   magic;        /* wakup magic */
};

typedef struct l4_wq_wakeup_msg l4_wq_wakeup_msg_t;

#define L4_WQ_WAKEUP_MSG_SIZE L4_IPC_DOPE(3,0)

/* prototypes */
extern inline void __wq_suspend_thread(l4_threadid_t wakeup_thread);
extern inline void __wq_wakeup_thread(l4_threadid_t tid, 
				      l4_threadid_t new_wait);

/*****************************************************************************
 * suspend thread                                                            *
 *****************************************************************************/
extern inline void __wq_suspend_thread(l4_threadid_t wakeup_thread)
{
  int error;
  l4_wq_wakeup_msg_t wq_msg;
  l4_msgdope_t result;
  
  wq_msg.size = L4_WQ_WAKEUP_MSG_SIZE;

  while (1)
    {
#if 0
      ko('s');
#endif

      error = l4_ipc_receive(wakeup_thread,&wq_msg,
				  &wq_msg.new_wait.lh.low,
				  &wq_msg.new_wait.lh.high,
				  L4_IPC_NEVER,&result);

#if 0
      ko('w');
#endif

      if (error)
	{
	  outstring("suspend_thread: IPC error ");
	  outhex16(error);
	  outstring("\n\r");
	  continue;
	}

      if ((wq_msg.magic == WQ_WAKEUP_MAGIC) && 
	  (l4_thread_equal(wq_msg.new_wait,L4_INVALID_ID)))
	return;

      if (wq_msg.magic == WQ_WAKEUP_MAGIC)
	{
	  outstring("new wakeup thread: ");
	  outhex16(wq_msg.new_wait.id.task);
	  outstring(".");
	  outhex16(wq_msg.new_wait.id.lthread);
	  outstring("\n\r");

	  wakeup_thread = wq_msg.new_wait;
	}
      else
	{
	  outstring("suspend_thread: ");
	  outstring("bad wakeup message!\n\r");
	}
    }
}

/*****************************************************************************
 * wake up next thread in wait queue                                         *
 *****************************************************************************/
extern inline void __wq_wakeup_thread(l4_threadid_t tid, 
				      l4_threadid_t new_wait)
{
  int error;
  l4_msgdope_t result;
  l4_wq_wakeup_msg_t wq_msg;
  
#if 0
  outstring("__wq_wakeup_thread: tid ");
  outhex16(tid.id.task);
  outstring(".");
  outhex16(tid.id.lthread);
  outstring(", new_wait ");
  outhex16(new_wait.id.task);
  outstring(".");
  outhex16(new_wait.id.lthread);
  outstring("\n\r");
#endif

  /* send wakeup message */
  wq_msg.size = wq_msg.send = L4_WQ_WAKEUP_MSG_SIZE;
  wq_msg.new_wait = new_wait;
  wq_msg.magic = WQ_WAKEUP_MAGIC;

  error = l4_ipc_send(tid,&wq_msg,
			   wq_msg.new_wait.lh.low,
			   wq_msg.new_wait.lh.high,
			   WQ_WAKEUP_TIMEOUT,&result);
  
  if (error)
    {
      outstring("wakeup_thread: ");
      outstring("wakeup failed (");
      outhex16(l4_myself().id.task);
      outstring(".");
      outhex16(l4_myself().id.lthread);
      outstring(" -> ");
      outhex16(tid.id.task);
      outstring(".");
      outhex16(tid.id.lthread);
      outstring(", ");
      outhex16(error);
      outstring(")\n\r");
    }
}

/*****************************************************************************
 * acquire lock                                                              *
 *****************************************************************************/
inline void l4_wq_lock(l4_wait_queue_t *wq)
{
  unsigned long flags;
  l4_wait_queue_entry_t wqe;
  l4_threadid_t wakeup_thread;

  __save_flags(flags);
  __cli();
  
#if 0
  outstring("l4_wq_lock: owner ");
  outhex16(wq->owner.id.task);
  outstring(".");
  outhex16(wq->owner.id.lthread);
  outstring("\n\r");
#endif

  if (l4_is_invalid_id(wq->owner))
    {
      /* lock free */
      wq->owner = l4_myself();
      __restore_flags(flags);
      return;
    }

  if (l4_thread_equal(wq->owner,l4_myself()))
    {
      /* I'm the owner */
      outstring("already locked\n\r");
      __restore_flags(flags);
      return;
    }
      
  wqe.thread = l4_myself();
  wqe.next = NULL;

  if (!wq->head)
    {
      /* wait queue empty */
      wq->head = wq->tail = &wqe;
      wakeup_thread = wq->owner;
    }
  else
    {
      /* insert thread into wait queue */
      wakeup_thread = wq->tail->thread;
      wq->tail->next = &wqe;
      wq->tail = &wqe;
    }

  __wq_suspend_thread(wakeup_thread);

  /* acquire lock */
  wq->owner = l4_myself();

  /* remove thread from wait queue */
  if (wq->head != &wqe)
    {
      outstring("lock: wait queue destroyed!\n\r");
      enter_kdebug("wq_lock");
      __restore_flags(flags);
      return;
    }

  wq->head = wq->head->next;
  if (!wq->head) 
    wq->tail = NULL;

  /* done */
  __restore_flags(flags);
}

/*****************************************************************************
 * release lock                                                              *
 *****************************************************************************/
inline void l4_wq_unlock(l4_wait_queue_t *wq)
{
  unsigned long flags;
  l4_threadid_t tid;

  __save_flags(flags);
  __cli();

#if 0
  outstring("l4_wq_unlock: owner ");
  outhex16(wq->owner.id.task);
  outstring(".");
  outhex16(wq->owner.id.lthread);
  outstring("\n\r");
#endif
  
  if (!l4_thread_equal(wq->owner,l4_myself()))
    {
      outstring("unlock: ");
      outstring("what's that: owner != me\n");
      enter_kdebug("wq_unlock");
      __restore_flags(flags);
      return;
    }

  if (!wq->head)
    {
      /* empty wait queue, release lock */
      wq->owner = L4_INVALID_ID;
      __restore_flags(flags);
      return;
    }

  /* wake up first thread in wait queue */
  tid = wq->head->thread;
  __wq_wakeup_thread(tid,L4_INVALID_ID);

  /* done */
  __restore_flags(flags);
}

/*****************************************************************************
 * remove thread from wait queue                                             *
 *****************************************************************************/
inline void l4_wq_remove_myself(l4_wait_queue_t *wq)
{
  unsigned long flags;
  l4_threadid_t me = l4_myself();
  l4_wait_queue_entry_t *wqe,*last;

  __save_flags(flags);
  __cli();

  if (l4_thread_equal(wq->owner,me))
    {
      outstring("l4_wq_remove_myself: ");
      outstring("I'm the owner\n\r");
      __restore_flags(flags);
      l4_wq_unlock(wq);
      return;
    }

  /* search myself in wait queue */
  last = NULL;
  wqe = wq->head;

  while (wqe)
    {
      if (l4_thread_equal(me,wqe->thread))
	{
	  outstring("l4_wq_remove_myself: ");
	  outstring("found myself ");

	  if (wqe == wq->head)
	    {
	      /* remove entry from wait queue head */
	      outstring("at head\n\r");
	      wq->head = wq->head->next;
	      if (!wq->head) 
		wq->tail = NULL;
	      else
		__wq_wakeup_thread(wq->head->thread,wq->owner);
	    }
	  else if (wqe == wq->tail)
	    {
	      /* remove entry from wait queue end */
	      outstring("at tail\n\r");
	      wq->tail = last;
	      wq->tail->next = NULL;
	    }
	  else
	    {
	      /* remove entry from wait queue */
	      outstring("in list\n\r");
	      last->next = wqe->next;
	      __wq_wakeup_thread(last->next->thread,last->thread);
	    }

	  /* done */
	  __restore_flags(flags);
	  return;
	}
      else
	{
	  last = wqe;
	  wqe = wqe->next;
	}
    }

  /* nothing found */
  outstring("l4_wq_remove_myself: ");
  outstring("nothing found\n\r");

  __restore_flags(flags);
}
