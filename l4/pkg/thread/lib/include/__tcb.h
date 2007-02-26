/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/include/__tcb.h
 * \brief  Thread control block.
 *
 * \date   09/02/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _THREAD___TCB_H
#define _THREAD___TCB_H

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/cdefs.h>
#include <l4/lock/lock.h>
#include <l4/dm_generic/types.h>

/* private includes */
#include <l4/thread/thread.h>
#include "__memory.h"
#include "__stacks.h"
#include "__config.h"

/*****************************************************************************
 *** thread control block
 *****************************************************************************/

/**
 * thread control block 
 */
typedef struct l4th_tcb
{
  l4lock_t               lock;          ///< TCB lock

  l4_uint16_t            state;         ///< thread state
  l4_uint16_t            flags;         ///< flags

  l4thread_t             id;            ///< user thread id
  struct l4th_tcb *      parent;        ///< parent thread
  l4_threadid_t          l4_id;         ///< L4 thread id

  l4th_mem_desc_t        stack;         ///< stack memory descriptor

  l4_prio_t              prio;          ///< thread priority

  /// thread data pointers
  void *                 data[L4THREAD_MAX_DATA_KEYS];
 
  /* thread startup */
  l4thread_fn_t          func;          ///< thread function
  void *                 startup_data;  ///< thread startup data

  void *                 return_data;   ///< thread startup return data

  /* thread exit */
  l4thread_exit_desc_t * exit_fns;      ///< exit functions
} l4th_tcb_t;

/* thread state */
#define TCB_UNUSED           0x0000     ///< TCB / thread unused
#define TCB_SETUP            0x0001     ///< startup in progress
#define TCB_ACTIVE           0x0002     ///< thread running
#define TCB_SHUTDOWN         0x0004     ///< shutdown in progress
#define TCB_RESERVED         0x0008     ///< TCB / thread reserved

/* flags */
#define TCB_ALLOCATED_STACK  0x0001     ///< stack allocated by thread lib

/*****************************************************************************
 *** TCB table
 *****************************************************************************/

/// TCB table
extern l4th_tcb_t * l4th_tcbs;

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/* initialization */
int
l4th_tcb_init(void);

/* TCB allocation */
int
l4th_tcb_allocate(l4th_tcb_t ** tcb);

int
l4th_tcb_allocate_id(l4thread_t thread, l4th_tcb_t ** tcb);

void
l4th_tcb_deallocate(l4th_tcb_t * tcb);

void
l4th_tcb_free(l4th_tcb_t * tcb);

int
l4th_tcb_reserve(l4thread_t thread);

/* thread ids */
L4_INLINE l4thread_t
l4th_tcb_get_current_id(void);

l4thread_t
l4th_tcb_get_current_id_slow(void);

/* TCB access */
L4_INLINE l4th_tcb_t *
l4th_tcb_get(l4thread_t thread);

L4_INLINE l4th_tcb_t *
l4th_tcb_get_active(l4thread_t thread);

L4_INLINE l4th_tcb_t *
l4th_tcb_get_active_locked(l4thread_t thread);

L4_INLINE l4th_tcb_t *
l4th_tcb_get_current(void);

L4_INLINE l4th_tcb_t *
l4th_tcb_get_current_locked(void);

L4_INLINE void
l4th_tcb_unlock(l4th_tcb_t * tcb);

/*****************************************************************************
 *** implementation
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Return id of current thread
 *	
 * \return Current thread id, #L4THREAD_INVALID_ID if not found
 */
/*****************************************************************************/ 
L4_INLINE l4thread_t
l4th_tcb_get_current_id(void)
{
  int stack_id = l4th_stack_get_current_id();
  
  if (stack_id >= 0)
    return stack_id;
  else
    return l4th_tcb_get_current_id_slow();
}

/*****************************************************************************/
/**
 * \brief  Return TCB of thread, test if state == TCB_ACTIVE
 * 
 * \param  thread        Thread id
 *	
 * \return TCB, NULL if invalid thread id
 */
/*****************************************************************************/ 
L4_INLINE l4th_tcb_t *
l4th_tcb_get(l4thread_t thread)
{
  if ((thread < 0) || (thread >= l4thread_max_threads))
    return NULL;

  return &l4th_tcbs[thread];
}

/*****************************************************************************/
/**
 * \brief  Return TCB of thread, test if state == TCB_ACTIVE
 * 
 * \param  thread        Thread id
 *	
 * \return TCB, NULL if invalid thread id or thread not active.
 */
/*****************************************************************************/ 
L4_INLINE l4th_tcb_t *
l4th_tcb_get_active(l4thread_t thread)
{
  if ((thread < 0) || (thread >= l4thread_max_threads))
    return NULL;
  
  if (l4th_tcbs[thread].state != TCB_ACTIVE)
    return NULL;
  else
    return &l4th_tcbs[thread];
}

/*****************************************************************************/
/**
 * \brief  Return TCB of thread, test if state == TCB_ACTIVE and lock TCB
 * 
 * \param  thread        Thread id
 *	
 * \return TCB, NULL if invalid thread id or thread not active.
 */
/*****************************************************************************/ 
L4_INLINE l4th_tcb_t *
l4th_tcb_get_active_locked(l4thread_t thread)
{
  if ((thread < 0) || (thread >= l4thread_max_threads))
    return NULL;

  /* lock TCB */
  l4lock_lock(&l4th_tcbs[thread].lock);

  if (l4th_tcbs[thread].state != TCB_ACTIVE)
    {
      l4lock_unlock(&l4th_tcbs[thread].lock);
      return NULL;
    }
  else
    return &l4th_tcbs[thread];
}

/*****************************************************************************/
/**
 * \brief  Return TCB of current thread
 *	
 * \return TCB, NULL if current thread not found in TCB table
 */
/*****************************************************************************/ 
L4_INLINE l4th_tcb_t *
l4th_tcb_get_current(void)
{
  l4thread_t thread = l4th_tcb_get_current_id();

  if ((thread < 0) || (thread >= l4thread_max_threads))
    return NULL;
  
  return &l4th_tcbs[thread];
}

/*****************************************************************************/
/**
 * \brief  Return TCB of current thread, lock TCB
 *	
 * \return TCB, NULL if current thread not found in TCB table
 */
/*****************************************************************************/ 
L4_INLINE l4th_tcb_t *
l4th_tcb_get_current_locked(void)
{
  l4thread_t thread = l4th_tcb_get_current_id();

  if ((thread < 0) || (thread >= l4thread_max_threads))
    return NULL;
  
  /* lock TCB */
  l4lock_lock(&l4th_tcbs[thread].lock);

  return &l4th_tcbs[thread];
}

/*****************************************************************************/
/**
 * \brief  Unlock TCB
 * 
 * \param  tcb           Thread control block
 */
/*****************************************************************************/ 
L4_INLINE void
l4th_tcb_unlock(l4th_tcb_t * tcb)
{
  if (tcb == NULL)
    return;

  /* unlock TCB */
  l4lock_unlock(&tcb->lock);
}

#endif /* !_THREAD___TCB_H */
