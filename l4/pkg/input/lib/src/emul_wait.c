/* $Id$ */
/*****************************************************************************/
/**
 * \file   input/lib/src/emul_wait
 * \brief  L4INPUT: Linux Wait queue emulation
 *
 * \date   2005/05/24
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * I've no idea if this is really needed.
 */
/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <stdio.h>
#include <l4/sys/ipc.h>
#include <l4/thread/thread.h>
#include <l4/util/util.h>
#include <l4/util/l4_macros.h>
#include <linux/wait.h>

l4_threadid_t wait_thread;

#define dbg(format, ...)	// printf(format, __VA_ARGS__)

/** Enqueue entry into list. */
static void
enqueue_entry(wait_queue_head_t *wqh, wait_queue_entry_t *e)
{
  if (!wqh->first)
    wqh->first = e;
  else
    {
      wait_queue_entry_t *w;
      for (w=wqh->first; w->next; w=w->next)
	;
      w->next = e;
    }
}

/** Enqueue list into main list. */
static void
enqueue_head(wait_queue_head_t **wqm, wait_queue_head_t *h)
{
  if (!*wqm)
    *wqm = h;
  else
    {
      wait_queue_head_t *w = *wqm;
      for (;;)
	{
	  if (w == h)
	    return;
	  if (!w->next)
	    {
	      w->next = h;
	      return;
	    }
	  w = w->next;
	}
    }
}

/** Dequeue list from main list. */
static void
dequeue_head(wait_queue_head_t **wqm, wait_queue_head_t *h)
{
  for (; *wqm; *wqm=(*wqm)->next)
    {
      if (*wqm == h)
	{
	  *wqm = h->next;
	  return;
	}
    }
}

/** Wakeup all threads enqueued in list. */
static void
wakeup(wait_queue_head_t *h)
{
  wait_queue_entry_t *e;
  l4_msgdope_t result;
  l4_threadid_t tid;

  for (e=h->first; e;)
    {
      dbg("    wakeup entry %08x\n", (unsigned)e);
      dbg("      ("l4util_idfmt", next=%08x)\n", 
	  l4util_idstr(e->tid), (unsigned)e->next);
      tid = e->tid;
      e   = e->next;

      l4_ipc_send(tid, L4_IPC_SHORT_MSG, 0, 0, L4_IPC_NEVER, &result);
    }
  h->first = 0;
}

/** The waiter thread. */
static void
__wait_thread(void *ignore)
{
  l4_threadid_t src;
  l4_msgdope_t result;
  l4_umword_t dw1, dw2;
  int error, e, m;
  wait_queue_head_t *main_queue = 0;

  l4thread_started(NULL);
  l4util_micros2l4to(1000, &e, &m);

  for (;;)
    {
      error = l4_ipc_wait(&src, L4_IPC_SHORT_MSG, &dw1, &dw2,
			  main_queue ? L4_IPC_TIMEOUT(0, 0, m, e, 0, 0)
			             : L4_IPC_NEVER,
			  &result);
      if (error == 0)
	{
	  /* got request */
	  if (dw1 != 0)
	    {
	      /* go sleep, append at wait queue */
	      wait_queue_head_t  *h = (wait_queue_head_t*)dw1;
	      wait_queue_entry_t *e = (wait_queue_entry_t*)dw2;

	      dbg("enqueue "l4util_idfmt" into queue %08x entry %08x\n",
		  l4util_idstr(src), dw1, dw2);

	      e->tid  = src;
	      e->next = 0;

	      /* append entry to wait queue */
	      enqueue_entry(h, e);
	      /* append wait queue to main queue if necessary */
	      enqueue_head(&main_queue, h);

	      dbg("  queue now ");
	      for (e=h->first; e; e=e->next)
		dbg("%08x ", (unsigned)e);
	      dbg("\n");
	    }
	  else
	    {
	      /* wakeup */
	      wait_queue_head_t *h = ((wait_queue_head_t*)dw2);
	      dbg("wakeup queue %08x\n", dw2);
	      /* wakeup waiting threads of wait queue */
	      wakeup(h);
	      /* dequeue wait queue from main queue */
	      dequeue_head(&main_queue, h);
	      dbg("  main=%08x\n", (unsigned)main_queue);
	      l4_ipc_send(src, L4_IPC_SHORT_MSG, 0, 0, L4_IPC_NEVER, &result);
	    }
	}
      else if (error == L4_IPC_RETIMEOUT)
	{
	  /* timeout, wakeup all queues */
	  wait_queue_head_t *h;
	  dbg("wakup all queues\n");
	  for (h=main_queue; h; h=h->next)
	    {
	      dbg("  wakeup queue %08x\n", (unsigned)h);
	      wakeup(h);
	    }
	  main_queue = 0;
	}
      else
	{
	  /* ??? */
	}
    }
}

/** Called if a specific wait queue should be woken up. */
void
wake_up(wait_queue_head_t *wq)
{
  int error;
  l4_umword_t dummy;
  l4_msgdope_t result;

  error = l4_ipc_call(wait_thread,
		      L4_IPC_SHORT_MSG, 0, (l4_umword_t)wq,
		      L4_IPC_SHORT_MSG, &dummy, &dummy,
		      L4_IPC_NEVER, &result);
}

/** Initialization. */
void
l4input_internal_wait_init(void)
{
  l4thread_t t = l4thread_create_long(L4THREAD_INVALID_ID,
				      (l4thread_fn_t) __wait_thread,
				      ".wait",
				      L4THREAD_INVALID_SP,
				      L4THREAD_DEFAULT_SIZE,
				      255,
				      NULL,
				      L4THREAD_CREATE_SYNC);
  wait_thread = l4thread_l4_id(t);
}
