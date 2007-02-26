/**
 * \file	roottask/server/src/irq_thread.c
 * \brief	thread handling for IRQ probing
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/
#include <stdio.h>

#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>

#include "irq_thread.h"
#include "rmgr.h"

#define __IRQ_STACKSIZE (256)
#define RMGR_IRQ_LTHREAD 16	/* base thread for IRQ threads */

/* stacks for irq-threads */
static char __irq_stacks[RMGR_IRQ_MAX * __IRQ_STACKSIZE];

void
__irq_thread(unsigned irq);

static int
irq_attach(unsigned irq)
{
  l4_umword_t dummy;
  int err;
  l4_msgdope_t result;
  l4_threadid_t irq_th;

  /* first, register to the irq */
  l4_make_taskid_from_irq(irq, &irq_th);

  err = l4_ipc_receive(irq_th, L4_IPC_SHORT_MSG, &dummy, &dummy,
		       L4_IPC_RECV_TIMEOUT_0, &result);

  return (err == L4_IPC_RETIMEOUT);
}

static int
irq_detach(unsigned irq)
{
  l4_umword_t dummy;
  l4_msgdope_t result;

  l4_ipc_receive(L4_NIL_ID, L4_IPC_SHORT_MSG, &dummy, &dummy,
		 L4_IPC_RECV_TIMEOUT_0, &result);

  return 1;
}

/* code for irq-thread */
void
__irq_thread(unsigned irq)
{
  l4_umword_t d1, d2, r1;
  l4_msgdope_t result;
  int err;
  l4_threadid_t t;

  int is_failure = 1, is_attached = 0;

  /* first, register to the irq */
  if (irq_attach(irq))
    {
      /* success! */
      is_failure = 0;
      is_attached = 1;
    }

  /* shake hands with the main thread who started and initialized us */
  err = l4_ipc_reply_and_wait(myself,
			      L4_IPC_SHORT_MSG, is_failure, 0,
			      &t, L4_IPC_SHORT_MSG, &d1, &d2,
			      L4_IPC_NEVER, &result);

  for (;;)
    {
      while (! err)
	{
	  if (t.id.task != myself.id.task)
	    break;		/* silently drop the request */

	  r1 = -1;

	  if (is_failure && d1)
	    {
	      /* we couldn't attach previous time -- try again */
	      if (irq_attach(irq))
		{
		  is_failure = 0;
		  is_attached = 1;
		}
	    }

	  if (! is_failure)
	    {
	      if (is_attached && d1)
		{
		  /* client wants to allocate irq -- detach it */
		  r1 = is_attached = irq_detach(irq) ? 0 : 1;
		}
	      else if (!is_attached && !d1)
		{
		  /* client wants to free irq -- attach */
		  r1 = is_failure = irq_attach(irq) ? 0 : 1;
		  is_attached = !is_failure;
		}
	    }

	  err = l4_ipc_reply_and_wait(t, L4_IPC_SHORT_MSG, r1, 0,
				      &t, L4_IPC_SHORT_MSG, &d1, &d2,
				      L4_IPC_NEVER, &result);
	}

      err = l4_ipc_wait(&t, L4_IPC_SHORT_MSG, &d1, &d2,
			L4_IPC_NEVER, &result);

    }
}



/* start a thread */
int
irq_get(int i)
{
  unsigned *sp;
  int error;
  l4_umword_t code, dummy;
  l4_msgdope_t result;
  l4_threadid_t t, preempter, pager;

  t = myself;
  t.id.lthread = LTHREAD_NO_IRQ(i);
  sp = (unsigned *)(__irq_stacks + (i + 1) * __IRQ_STACKSIZE);

      *--sp = i;		/* pass irq number as argument to thr func */
      *--sp = 0;		/* faked return address */
      preempter = my_preempter;
      pager = my_pager;

      l4_thread_ex_regs(t,
	                (l4_umword_t) __irq_thread,
			(l4_umword_t) sp,
                        &preempter, &pager,
			&dummy, &dummy, &dummy);

      /* handshake and status check */
      error = l4_ipc_receive(t, L4_IPC_SHORT_MSG, &code, &dummy,
                             L4_IPC_NEVER, &result);

      if (error)
	printf("can't find IRQ thread\n");

  return code;
}

