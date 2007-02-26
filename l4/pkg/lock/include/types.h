/* $Id$ */
/*****************************************************************************/
/**
 * \file  lock/include/l4/lock/types.h
 * \brief L4 lock data types.
 *
 * \date   12/30/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * L4 lock data types.
 */
/*****************************************************************************/
#ifndef _L4_LOCK_TYPES_H
#define _L4_LOCK_TYPES_H

/* L4env includes */
#include <l4/env/cdefs.h>
#include <l4/semaphore/semaphore.h>
#include <l4/thread/thread.h>

/* Lib includes */
#include <l4/lock/internal.h>

/*****************************************************************************
 * data types
 *****************************************************************************/

__BEGIN_DECLS;

/**
 * \brief   Lock type
 * \ingroup api
 */
typedef struct l4lock l4lock_t;

__END_DECLS;

/*****************************************************************************
 * defines 
 *****************************************************************************/

/**
 * \brief   Lock initializer (locked), use to initialize locks in nested 
 *          structures
 * \param   t            Initial owner of the lock 
 * \ingroup api
 */
#define L4LOCK_LOCKED_INITIALIZER(t) \
	{(L4SEMAPHORE_LOCKED_INITIALIZER),(t),1}

/**
 * \brief   Lock value (locked), use to initialize plain lock variables
 * \param   t            Initial owner of the lock 
 * \ingroup api
 */
#define L4LOCK_LOCKED(t) ((l4lock_t) L4LOCK_LOCKED_INITIALIZER(t))

/** 
 * \brief   Lock initializer (unlocked), use to initialize locks in nested
 *          structures
 * \ingroup api
 */
#define L4LOCK_UNLOCKED_INITIALIZER \
	{L4SEMAPHORE_UNLOCKED_INITIALIZER, (L4THREAD_INVALID_ID),0}

/** 
 * \brief   Lock value (unlocked), use to initialize plain lock variables
 * \ingroup api
 */
#define L4LOCK_UNLOCKED  ((l4lock_t) L4LOCK_UNLOCKED_INITIALIZER)

#endif /* !_L4_LOCK_TYPES_H */
