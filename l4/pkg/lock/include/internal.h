/* $Id$ */
/*****************************************************************************/
/**
 * \file  lock/include/l4/internal.h
 * \brief Non-public data types 
 *
 * \date   12/30/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * The reason to put them into a separate file is to ease the usage of doxygen
 * to generate both the reference manual and source code documentation.
 * Since internal.h contains implementation-specific stuff, it is not 
 * included in the sources of the reference manual.
 */
/*****************************************************************************/
#ifndef _L4_LOCK_INTERNAL_H
#define _L4_LOCK_INTERNAL_H

/* L4 includes */
#include <l4/env/cdefs.h>
#include <l4/semaphore/semaphore.h>
#include <l4/thread/thread.h>

/*****************************************************************************
 * type definitions
 *****************************************************************************/

__BEGIN_DECLS;

/**
 * \brief Lock structure.
 */
struct l4lock
{
  l4semaphore_t        sem;        /**< semaphore */
  volatile l4thread_t  owner;      /**< lock owner */
  volatile int         ref_count;  /**< reference counter, it is used to 
				    **  allow reentering of a lock by the 
				    **  same thread */
};

__END_DECLS;

#endif /* _L4_LOCK_INTERNAL_H */
