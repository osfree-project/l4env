/**
 * \file   l4util/include/thread.h
 * \brief  Low-level Thread Functions
 *
 * \date   1997
 * \author Sebastian Schönberg */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __L4_THREAD_H
#define __L4_THREAD_H

#include <l4/sys/types.h>

EXTERN_C_BEGIN

/** \defgroup thread Low-Level Thread Functions */

/** Create an L4 thread.
 * \ingroup thread
 * \note  You should prefer to use the \b thread package of the L4 environment.
 * \param thread_no  number of thread to create
 * \param function   thread code
 * \param stack      initial value of stack pointer
 * \return thread id of created stack
 */
l4_threadid_t
l4util_create_thread (int thread_no, void (*function)(void), void *stack);

/** Attach to hardware interrupt.
 * \ingroup thread
 * \param irq        number of IRQ to attach to
 */
l4_threadid_t
l4util_attach_interrupt (int irq);

/** Detach from hardware interrupt
 * \ingroup thread
 */
void
l4util_detach_interrupt (void);

EXTERN_C_END

#endif /* __L4_THREAD_H */
