/* $Id$ */
/*****************************************************************************/
/**
 * \file   semaphore/include/semaphore.h
 * \brief  Semaphore implementation, user programming interface
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
#ifndef _L4_SEMAPHORE_SEMAPHORE_H
#define _L4_SEMAPHORE_SEMAPHORE_H

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/cdefs.h>
#include <l4/thread/thread.h>
#include <l4/util/util.h>

/*****************************************************************************
 *** configuration 
 *****************************************************************************/

/**
 * use send-only IPC to wakeup blocked threads 
 */
#define L4SEMAPHORE_SEND_ONLY_IPC     0

/**
 * use assembler version of up/down
 */
#define L4SEMAPHORE_ASM               1

/*****************************************************************************
 *** types                                                                         
 *****************************************************************************/

/**
 * Semaphore type
 */
typedef struct l4semaphore
{
  volatile int  counter;      /**< semaphore counter */
  int           pending;      /**< wakeup notification pending counter */
  void *        queue;        /**< wait queue */
} l4semaphore_t;

/*****************************************************************************
 *** global data
 *****************************************************************************/

/**
 * Semaphore thread id
 */
extern l4_threadid_t l4semaphore_thread_l4_id;

/*****************************************************************************
 *** defines
 *****************************************************************************/

/** 
 * \brief   Semaphore initializer, use this to initialize semaphores in 
 *          nested structures
 * \ingroup api_sem
 * \param   x            Initial value for semaphore counter
 */
#define L4SEMAPHORE_INITIALIZER(x)  {(x), 0, NULL}

/** 
 * \brief   Semaphore value generator, use this to initialize plain semaphores
 * \ingroup api_sem
 * \param   x            Initial value for semaphore counter
 */
#define L4SEMAPHORE_INIT(x)	\
	((l4semaphore_t)L4SEMAPHORE_INITIALIZER(x))

/** 
 * \brief   Semaphore initializer, initial count 0 (semaphore locked) 
 * \ingroup api_sem
 */
#define L4SEMAPHORE_LOCKED_INITIALIZER  \
	L4SEMAPHORE_INITIALIZER(0)

/** 
 * \brief   Locked semaphore value, initial count 0
 * \ingroup api_sem
 */
#define L4SEMAPHORE_LOCKED \
	((l4semaphore_t)L4SEMAPHORE_LOCKED_INITIALIZER)

/**
 * \brief   Semaphore initializer, initial count 1 (semaphore unlocked)
 * \ingroup api_sem
 */
#define L4SEMAPHORE_UNLOCKED_INITIALIZER \
	L4SEMAPHORE_INITIALIZER(1)

/**
 * \brief   Unlocked semaphore value, initial count 1
 * \ingroup api_sem
 */
#define L4SEMAPHORE_UNLOCKED \
	((l4semaphore_t)L4SEMAPHORE_UNLOCKED_INITIALIZER)


/* Semaphore thread IPC commands (dw0), check assembler implementation 
 * if changed! */
#define L4SEMAPHORE_BLOCK    0x00000001   ///< block calling thread
#define L4SEMAPHORE_RELEASE  0x00000002   ///< wakeup other threads

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

__BEGIN_DECLS;

/*****************************************************************************/
/**
 * \brief   Library initialization
 * \ingroup api_init
 *
 * \return  0 on success, -1 if setup failed
 * 
 * Setup semaphore thread. This function is usually called during the setup 
 * of a task by the environment setup routine, applications do not need to 
 * call it explicitly.
 */
/*****************************************************************************/
int
l4semaphore_init(void);

/*****************************************************************************/
/**
 * \brief   Set semaphore thread priority.
 * \ingroup api_init
 *
 * \param    prio        Priority
 *	
 * \return 0 on success, error code if failed.
 */
/*****************************************************************************/ 
int
l4semaphore_set_thread_prio(l4_prio_t prio);

/*****************************************************************************/
/**
 * \brief   Decrement semaphore counter, block if result is \< 0
 * \ingroup api_sem
 *
 * \param   sem          Semaphore structure
 *
 * Decrement semaphore counter by 1. If the result is \< 0, l4semaphore_down 
 * blocks and waits for the release of the semaphore. 
 */
/*****************************************************************************/
volatile extern inline void 
l4semaphore_down(l4semaphore_t * sem);

/*****************************************************************************/
/**
 * \brief   Decrement semaphore counter, block for a given time 
 *          if result is \< 0
 * \ingroup api_sem
 *
 * \param   sem          Semaphore structure
 * \param   time         Timeout (in ms)
 * \return  0 if semaphore successfully decremented within given time, 
 *          != 0 otherwise.    
 *
 * Decrement semaphore counter by 1. If the result is \< 0, 
 * \a l4semaphore_down_timed blocks for \a time ms and waits for the release 
 * of the semaphore. 
 */
/*****************************************************************************/
volatile extern inline int
l4semaphore_down_timed(l4semaphore_t * sem, unsigned time);

/*****************************************************************************/
/**
 * \brief   Decrement semaphore counter, do not wait if result is \< 0
 * \ingroup api_sem
 *
 * \param   sem          Semaphore structure
 * \return  1 on success (counter decremented), 0 if semaphore already locked
 *
 * Try to decrement semaphore counter by 1, if the result would be \< 0, return
 * error instead.
 */
/*****************************************************************************/
volatile L4_INLINE int
l4semaphore_try_down(l4semaphore_t * sem);

/*****************************************************************************/
/**
 * \brief   Increment semaphore counter, wakeup next thread in wait queue
 * \ingroup api_sem
 * 
 * \param   sem          Semaphore structure
 * 
 * Increment semaphore counter by 1. If threads are enqueued in the wait queue, 
 * wakeup the first thread.
 */
/*****************************************************************************/
volatile L4_INLINE void
l4semaphore_up(l4semaphore_t * sem);

__END_DECLS;

/*****************************************************************************
 *** implementation
 *****************************************************************************/

#if L4SEMAPHORE_ASM
#  include <l4/semaphore/asm.h>
#else
#  include <l4/semaphore/generic.h>
#endif

#endif /* !_L4_SEMAPHORE_SEMAPHORE_H */
