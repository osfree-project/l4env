/* $Id$ */
/*****************************************************************************/
/**
 * \file   semaphore/include/ARCH-x86/L4API-l4v2/asm.h
 * \brief  Optimized assembler semaphore implementation, x86/L4v2 version
 *
 * \date   11/13/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _L4_SEMAPHORE_ASM_H
#define _L4_SEMAPHORE_ASM_H

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/util/atomic.h>
#include <l4/env/cdefs.h>
#include <l4/thread/thread.h>

/*****************************************************************************
 *** configuration
 *****************************************************************************/

/**
 * move block/wakeup handler to separate text section 
 */
#define L4SEMAPHORE_USE_LOCK_SECTION   0

/*****************************************************************************
 *** external functions
 *****************************************************************************/
__BEGIN_DECLS
/* internal use only */
void l4semaphore_restart_up(l4semaphore_t *sem,l4_msgdope_t lastresult);
void l4semaphore_restart_down(l4semaphore_t *sem,l4_msgdope_t lastresult);
__END_DECLS

/*****************************************************************************
 *** implementation
 *****************************************************************************/

/*****************************************************************************
 * decrement semaphore counter, block if semaphore locked
 *****************************************************************************/
L4_INLINE void 
l4semaphore_down(l4semaphore_t * sem)
{
  unsigned dummy;
  l4_msgdope_t result;

  __asm__ __volatile__ 
    (
     "decl    0(%%ecx)           \n\t"        /* decrement counter */
#if L4SEMAPHORE_USE_LOCK_SECTION
     "js      1f                 \n\t"

     ".section .text.lock,\"ax\" \n\t"        /* move handler for blocking to
				               * its own section, => no jumps 
				               * if we got the lock */
     "1:                         \n\t"
#else
     "jge     2f                 \n\t"
#endif

     "pushl   %%ebx              \n\t"
     "pushl   %%edx              \n\t"
     "pushl   %%ebp              \n\t"

     /* setup call to semaphore thread */
     "xorl    %%ebp,%%ebp        \n\t"        /* short receive */
     "movl    $1,%%edx           \n\t"        /* dw0 -> L4SEMAPHORE_BLOCK */
     "movl    %%ecx,%%ebx        \n\t"        /* dw1 -> semaphore */
     "xorl    %%ecx,%%ecx        \n\t"        /* timeout never, everything 
					       * else is already set */

     IPC_SYSENTER                             /* call semaphore thread */

     "popl    %%ebp              \n\t"
     "popl    %%edx              \n\t"
     "popl    %%ebx              \n\t"

#if L4SEMAPHORE_USE_LOCK_SECTION
     "jmp     2f                 \n\t"

     ".previous                  \n\t"        /* back to original section */
#endif
     "2:                         \n\t"
     :
     "=a" (result),                           /* EAX, 0, result dope */
     "=c" (dummy),                            /* ECX, 1 */
     "=D" (dummy),                            /* EDI, 2 */
     "=S" (dummy)                             /* ESI, 3 */
     :
     "a"  (L4_IPC_SHORT_MSG),                 /* EAX, short send */
     "c"  (sem),                              /* ECX, semaphore */
     "S"  (l4semaphore_thread_l4_id.raw),     /* ESI, id */
     "D"  (0)                                 /* EDI, msgtag */
     :
     "memory"
     );

#if L4SEMAPHORE_RESTART_IPC
  if (EXPECT_FALSE((signed)L4_IPC_ERROR(result)))
    l4semaphore_restart_down(sem,result);
#endif
}

/*****************************************************************************
 * increment semaphore counter, wakeup first thread waiting
 *****************************************************************************/
L4_INLINE void
l4semaphore_up(l4semaphore_t * sem)
{
  unsigned dummy;
  l4_msgdope_t result;

  __asm__ __volatile__
    (
     "incl    0(%%ecx)           \n\t"        /* increment counter */
#if L4SEMAPHORE_USE_LOCK_SECTION
     "jle     1f                 \n\t"

     ".section .text.lock,\"ax\" \n\t"        /* move handler for wakeup to
					       * its own section, => no jumps 
					       * if no one is waiting */
     "1:                         \n\t"
#else
     "jg      2f                 \n\t"
#endif

     "pushl   %%ebx              \n\t"
     "pushl   %%edx              \n\t"
     "pushl   %%ebp              \n\t"

     /* setup message to semaphore thread */
#if L4SEMAPHORE_SEND_ONLY_IPC
     "movl    $-1,%%ebp          \n\t"        /* no receive */
#else
     "xorl    %%ebp,%%ebp        \n\t"        /* short receive */
#endif
     "movl    $2,%%edx           \n\t"        /* dw0 -> L4SEMAPHORE_RELEASE */
     "movl    %%ecx,%%ebx        \n\t"        /* dw1 -> semaphore */
     "xorl    %%ecx,%%ecx        \n\t"        /* timeout never, everything 
					       * else is already set */
     
     IPC_SYSENTER                             /* do IPC */

     "popl    %%ebp              \n\t"
     "popl    %%edx              \n\t"
     "popl    %%ebx              \n\t"
#if L4SEMAPHORE_USE_LOCK_SECTION
     "jmp     2f                 \n\t"

     ".previous                  \n\t"        /* back to original section */
#endif
     "2:                         \n\t"
     :
     "=a" (result),                           /* EAX, 0, result dope */
     "=c" (dummy),                            /* ECX, 1 */
     "=D" (dummy),                            /* EDI, 2 */
     "=S" (dummy)                             /* ESI, 3 */
     :
     "a"  (L4_IPC_SHORT_MSG),                 /* EAX, short send */
     "c"  (sem),                              /* ECX, semaphore */
     "S"  (l4semaphore_thread_l4_id.raw),     /* ESI, id */
     "D"  (0)                                 /* EDI, msgtag */
     :
     "memory"
     );

#if L4SEMAPHORE_RESTART_IPC
  if (EXPECT_FALSE((signed)L4_IPC_ERROR(result)))
    l4semaphore_restart_up(sem,result);
#endif
}

/*****************************************************************************
 * decrement semaphore counter, return error if semaphore locked
 * no assambler version
 *****************************************************************************/
L4_INLINE int
l4semaphore_try_down(l4semaphore_t * sem)
{
  int old,tmp;

  /* try to decrement the semaphore counter */
  do
    {
      old = sem->counter;

      if (old <= 0)
	/* semaphore already locked */
	return 0;

      tmp = old - 1;
    }
  /* retry if someone else also modified the counter */
  while (!l4util_cmpxchg32((volatile l4_uint32_t *)&sem->counter,
			   (l4_uint32_t)old, (l4_uint32_t)tmp));

  /* decremented semaphore counter */
  return 1;
}

/*****************************************************************************
 * decrement semaphore counter, block for a given time if semaphore locked
 *****************************************************************************/
L4_INLINE int
l4semaphore_down_timed(l4semaphore_t * sem, unsigned timeout)
{
  int old,tmp,ret;
  l4_umword_t dummy;
  l4_msgdope_t result;

  /* decrement counter, check result */
  do
    {
      old = sem->counter;
      tmp = old - 1;
    }
  /* retry if someone else also modified the counter */
  while (!l4util_cmpxchg32((volatile l4_uint32_t *)&sem->counter,
			   (l4_uint32_t)old, (l4_uint32_t)tmp));

  if (tmp < 0)
    {
      /* we did not get the semaphore, block */
      ret = l4_ipc_call(l4semaphore_thread_l4_id,
			L4_IPC_SHORT_MSG, L4SEMAPHORE_BLOCKTIMED,
			(l4_umword_t)sem,
			L4_IPC_SHORT_MSG, &dummy, &dummy,
			l4_timeout(L4_IPC_TIMEOUT_NEVER, 
			  l4util_micros2l4to((signed)timeout*1000)),
			&result);

      if (ret != 0)
        {
          do 
            {
              old = sem->counter;
              tmp = old + 1;
            }
          /* retry if someone else also modified the counter */
          while (!l4util_cmpxchg32((volatile l4_uint32_t *)&sem->counter,
				   (l4_uint32_t)old, (l4_uint32_t)tmp));

          /* semaphore thread did not receive IPC, nothing to do*/
          if (ret == L4_IPC_SECANCELED || ret == L4_IPC_SETIMEOUT ||
              ret == L4_IPC_SEABORTED)
            return 1; 

          /* we had a timeout, notify the semaphore thread to
             remove us from queue*/
          while (ret != 0)
            ret = l4_ipc_send(l4semaphore_thread_l4_id, L4_IPC_SHORT_MSG,
                              L4SEMAPHORE_RELEASETIMED, (l4_umword_t)sem,
                              L4_IPC_NEVER, &result);

          return 1;
        }
    }
  return 0;
}

#endif /* !_L4_SEMAPHORE_ASM_H */
