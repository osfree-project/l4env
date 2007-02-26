/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/src/exit.c
 * \brief  Exit functions.
 *
 * \date   09/06/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 * 
 * The cleanup of a thread includes the release of its stack. Since we need
 * a valid stack pointer also after we released the thread stack, we must 
 * set the stack pointer to a special exit stack. We have only one such exit 
 * stack, the usage is synchronized through a simple busy-wait lock. 
 */
/*****************************************************************************/

/* L4env includes */
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/util/atomic.h>
#include <l4/util/macros.h>
#include <l4/env/errno.h>

/* private includes */
#include <l4/thread/thread.h>
#include "__thread.h"
#include "__tcb.h"
#include "__stacks.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/* exit stack */
static unsigned char exit_stack[L4THREAD_EXIT_STACK_SIZE];

/* exit stack lock */
static l4_uint32_t exit_stack_used = 0;

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Acquire exit stack lock. 
 * 
 * Try to set exit_stack_used to 1. If it is already set to 1, poll until
 * it is released. 
 */
/*****************************************************************************/ 
static inline void
__lock_exit_stack(void)
{
  l4_msgdope_t result;
  l4_umword_t dummy;

  /* try to get the lock */
  while (!cmpxchg32(&exit_stack_used,0,1))
    {
      /* wait 1 ms */
      l4_i386_ipc_receive(L4_NIL_ID,L4_IPC_SHORT_MSG,&dummy,&dummy,
			  L4_IPC_TIMEOUT(0,0,250,14,0,0),&result);
    }
}

/*****************************************************************************/
/**
 * \brief  Cleanup stack and block forever.
 * 
 * \param  tcb           Thread control block.
 *
 * Cleanup thread \a tcb and block L4 thread forever.
 */
/*****************************************************************************/ 
static void
__do_cleanup_and_block(l4th_tcb_t * tcb)
{
  /* release thread stack */
  if (tcb->flags & TCB_ALLOCATED_STACK)
    l4th_stack_free(&tcb->stack);

  /* deallocate thread */
  l4th_tcb_deallocate(tcb);

  /* do the final cleanup and block */
  __asm__ __volatile__(
    "leal  %0,%%eax          \n\t"  /* move address of tcb state to eax */
    "movl  $0,%1             \n\t"  /* unlock exit stack, no stack 
				     * references after this point */
    "movw  %2,(%%eax)        \n\t"  /* mark tcb unused */

    /* block forever */
    "1:                      \n\t"
    "movl  $0xffffffff,%%eax \n\t"  /* EAX = -1, no send operation */
    "xorl  %%ebp,%%ebp       \n\t"  /* EBP = 0, short closed wait */
    "xorl  %%esi,%%esi       \n\t"  /* ESI = 0, dest = nil id */
    "xorl  %%ecx,%%ecx       \n\t"  /* ECX = 0, timeout never */

    /* IPC */
    "int $0x30\n\t"		    /* no sysenter, as this potentially
    				       accesses the stack */

    "jmp   1b                \n\t"
    : 
    /* no output */
    :
    "m" (tcb->state),               /* 0, thread state */
    "m" (exit_stack_used),          /* 1, exit stack lock */
    "i" (TCB_UNUSED)                /* 2, 'unused' thread state */
    );
}

/*****************************************************************************/
/**
 * \brief  Thread exit 
 */
/*****************************************************************************/ 
static void
__do_exit(void)
{
  l4th_tcb_t * tcb;
  l4thread_exit_desc_t * exit_fn;
  l4_addr_t * sp;
  l4_threadid_t foo;
  l4_umword_t dummy;
  
  /* get TCB, this will never fail, checks are done in l4thread_exit resp.
   * l4thread_shutdown */
  tcb = l4th_tcb_get_current_locked();
  Assert(tcb != NULL);

  /* set thread state to TCB_ACTIVE again, the 'on_exit' functions should see
   * a valid TCB, but the state is set to TCB_SHUTDOWN in l4thread_shutdown
   * to avoid that someone else modifies the TCB while we set the thread
   * to __do_exit (the TCB must be unlocked to do that). */
  tcb->state = TCB_ACTIVE;

  /* call 'on_exit' functions */
  while (tcb->exit_fns != NULL)
    {
      /* remove exit function from list before we call it, this avoids
       * infinte recursions if exit function calls l4thread_exit again */
      exit_fn = tcb->exit_fns;
      tcb->exit_fns = exit_fn->next;

      /* call function */
      if (exit_fn->fn != NULL)
	exit_fn->fn(tcb->id,exit_fn->data);
    }

  /* now really exit */
  tcb->state = TCB_SHUTDOWN;

  /* unlock TCB, the TCB might be locked multiple times, e.g. if a thread 
   * calls l4thread_exit() and has locked itself (l4thread_lock()) */
  while (l4thread_equal(l4lock_owner(&tcb->lock),tcb->id))
    l4lock_unlock(&tcb->lock);

  /* set thread to exit stack and call __do_cleanup_and_block to finish exit */
  __lock_exit_stack();
  sp = (l4_addr_t *)&exit_stack[L4THREAD_EXIT_STACK_SIZE];
  *(--sp) = (l4_addr_t)tcb;
  *(--sp) = 0;
  
  foo = L4_INVALID_ID;
  l4_thread_ex_regs(tcb->l4_id,(l4_umword_t)__do_cleanup_and_block,
		    (l4_umword_t)sp,&foo,&foo,&dummy,&dummy,&dummy);
}

/*****************************************************************************
 *** l4thread user API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Exit current thread.
 */
/*****************************************************************************/ 
void
l4thread_exit(void)
{
  l4th_tcb_t * tcb;

  /* get TCB */
  tcb = l4th_tcb_get_current_locked();
  if ((tcb == NULL) || (tcb->state != TCB_ACTIVE))
    Error("l4thread: myself not found in TCB table!");
  else
    /* do exit */
    __do_exit();

  /* avoid compiler warning: 
   * l4thread_exit is declared 'noreturn', but __do_exit is not (__do_exit 
   * is mainly inline assambler, gcc cannot see that it does not return). 
   * To avoid the warning "`noreturn' function does return" we put an 
   * infinite loop here so gcc thinks we never return.
   */
  Panic("l4thread: __do_exit returned!");
  while(1);   
}

/*****************************************************************************/
/**
 * \brief  Shutdown thread.
 * 
 * \param  thread        Thread number
 *
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL invalid thread number
 */
/*****************************************************************************/ 
int 
l4thread_shutdown(l4thread_t thread)
{
  l4th_tcb_t * tcb, * me;
  l4_threadid_t foo;
  l4_umword_t dummy;

  /* lock myself, this avoids that someone else kills us while we are killing
   * the other thread */
  me = l4th_tcb_get_current_locked();

  /* get TCB */
  tcb = l4th_tcb_get_active_locked(thread);
  if (tcb == NULL)
    {
      l4th_tcb_unlock(me);
      return -L4_EINVAL;
    }

  /* set thread state to TCB_SHUTDOWN, this makes the thread invisible for 
   * other threads */
  tcb->state = TCB_SHUTDOWN;

  /* unlock TCB, the lock is reclaimed by the thread itself during the final 
   * cleanup in __do_exit, we might hold the lock several times, e.g. if the
   * caller has locked the thread (l4thread_lock()) */
  while (l4thread_equal(l4lock_owner(&tcb->lock),me->id))
    l4th_tcb_unlock(tcb);

  /* set thread to __do_exit */
  foo = L4_INVALID_ID;
  l4_thread_ex_regs(tcb->l4_id,(l4_umword_t)__do_exit,(l4_umword_t)-1,
		    &foo,&foo,&dummy,&dummy,&dummy);

  /* unlock myself, we are finished at this point */
  l4th_tcb_unlock(me);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Register exit function for current thread
 * 
 * \param  exit_fn       Exit function, it must be declared with the 
 *                       #L4THREAD_EXIT_FN macros
 * \param  data          Data pointer which will be passed to the exit 
 *                       function
 *	
 * \return 0 on success, error code otherwise
 *         - -#L4_EINVAL  invalid error function / thread
 */
/*****************************************************************************/ 
int
l4thread_on_exit(l4thread_exit_desc_t * exit_fn, void * data)
{
  l4th_tcb_t * tcb;

  if ((exit_fn == NULL) || (exit_fn->fn == NULL))
    return -L4_EINVAL;

  /* get current tcb */
  tcb = l4th_tcb_get_current_locked();
  if (tcb == NULL)
    return -L4_EINVAL;
  
  if (tcb->state != TCB_ACTIVE)
    {
      l4th_tcb_unlock(tcb);
      return -L4_EINVAL;
    }

  /* insert exit function */
  exit_fn->data = data;
  exit_fn->next = tcb->exit_fns;
  tcb->exit_fns = exit_fn;
  
  /* done */
  l4th_tcb_unlock(tcb);
  return 0;
}
