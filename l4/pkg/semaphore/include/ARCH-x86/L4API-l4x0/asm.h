/* $Id$ */
/*****************************************************************************/
/**
 * \file   semaphore/include/ARCH-x86/L4API-l4x0/asm.h
 * \brief  Optimized assembler semaphore implementation, x86/L4 x0 version
 *
 * \date   11/13/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * This file provides the user programming interface for the L4 semaphore 
 * implementation. 
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
 */
/*****************************************************************************/
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
 *** global data 
 *****************************************************************************/

/**
 * Semaphore thread id (32 bit version)
 */
extern l4_uint32_t l4semaphore_thread_l4_id32;

/*****************************************************************************
 *** implementation
 *****************************************************************************/

/*****************************************************************************
 * decrement semaphore counter, block if semaphore locked
 *****************************************************************************/
volatile L4_INLINE void 
l4semaphore_down(l4semaphore_t * sem)
{
  unsigned dummy;

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
     "pushl   %%eax              \n\t"
     "pushl   %%edx              \n\t"
     "pushl   %%edi              \n\t"
     "pushl   %%ebp              \n\t"

     /* setup call to semaphore thread */
     "xorl    %%eax,%%eax        \n\t"        /* short send */
     "xorl    %%ebp,%%ebp        \n\t"        /* short receive */
     "movl    $1,%%edx           \n\t"        /* dw0 -> L4SEMAPHORE_BLOCK */
     "movl    %%ecx,%%ebx        \n\t"        /* dw1 -> semaphore */
     "xorl    %%ecx,%%ecx        \n\t"        /* timeout never, everything 
					       * else is already set */

     IPC_SYSENTER                             /* call semaphore thread */

     "popl    %%ebp              \n\t"
     "popl    %%edi              \n\t"
     "popl    %%edx              \n\t"
     "popl    %%eax              \n\t"
     "popl    %%ebx              \n\t"

#if L4SEMAPHORE_USE_LOCK_SECTION
     "jmp     2f                 \n\t"

     ".previous                  \n\t"        /* back to original section */
#endif
     "2:                         \n\t"
     :
     "=c" (dummy),                            /* ECX, 0 */
     "=S" (dummy)                             /* ESI, 1 */
     :
     "c"  (sem),                              /* ECX, semaphore */
     "S"  (l4semaphore_thread_l4_id32)        /* ESI, semaphore thread id */
     :
     "memory"
     );
}

/*****************************************************************************
 * increment semaphore counter, wakeup first thread waiting
 *****************************************************************************/
volatile extern inline void
l4semaphore_up(l4semaphore_t * sem)
{
  unsigned dummy;

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
     "pushl   %%eax              \n\t"
     "pushl   %%edx              \n\t"
     "pushl   %%edi              \n\t"
     "pushl   %%ebp              \n\t"

     /* setup message to semaphore thread */
#if L4SEMAPHORE_SEND_ONLY_IPC
     "movl    $-1,%%ebp          \n\t"        /* no receive */
#else
     "xorl    %%ebp,%%ebp        \n\t"        /* short receive */
#endif
     "xorl    %%eax,%%eax        \n\t"        /* short send */     
     "movl    $2,%%edx           \n\t"        /* dw0 -> L4SEMAPHORE_RELEASE */
     "movl    %%ecx,%%ebx        \n\t"        /* dw1 -> semaphore */
     "xorl    %%ecx,%%ecx        \n\t"        /* timeout never, everything 
					       * else is already set */
     
     IPC_SYSENTER                             /* do IPC */

     "popl    %%ebp              \n\t"
     "popl    %%edi              \n\t"
     "popl    %%edx              \n\t"
     "popl    %%eax              \n\t"
     "popl    %%ebx              \n\t"
#if L4SEMAPHORE_USE_LOCK_SECTION
     "jmp     2f                 \n\t"

     ".previous                  \n\t"        /* back to original section */
#endif
     "2:                         \n\t"
     :
     "=c" (dummy),                            /* ECX, 0 */
     "=S" (dummy)                             /* ESI, 1 */
     :
     "c"  (sem),                              /* ECX, semaphore */
     "S"  (l4semaphore_thread_l4_id32)        /* ESI, semaphore thread id */
     :
     "memory"
     );
}

/*****************************************************************************
 * decrement semaphore counter, return error if semaphore locked
 * no assambler version
 *****************************************************************************/
volatile L4_INLINE int
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
  while (!cmpxchg32((l4_uint32_t *)&sem->counter,old,tmp));

  /* decremented semaphore counter */
  return 1;
}

/*****************************************************************************
 * decrement semaphore counter, block for a given time if semaphore locked
 *****************************************************************************/
volatile L4_INLINE int
l4semaphore_down_timed(l4semaphore_t * sem, unsigned time)
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
  while (!cmpxchg32((l4_uint32_t *)&sem->counter,old,tmp));

  if (tmp < 0)
    {
      int e, m;
      
      micros2l4to(time*1000, &e, &m);
      /* we did not get the semaphore, block */
      ret = l4_i386_ipc_call(l4semaphore_thread_l4_id,
			     L4_IPC_SHORT_MSG,L4SEMAPHORE_BLOCK,
			     (l4_umword_t)sem,
			     L4_IPC_SHORT_MSG,&dummy,&dummy,
			     L4_IPC_TIMEOUT(0, 0, m, e, 0, 0),&result);
      if (ret != 0)
	{
          /* we had a timeout, do semaphore_up to compensate */
          l4semaphore_up(sem);
        
	  return 1;
	}
    }
  return 0;
}

#endif /* !_L4_SEMAPHORE_ASM_H */
