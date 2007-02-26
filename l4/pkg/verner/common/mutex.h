/*
 * \brief   Mutex handling for VERNER's sync component
 * \date    2004-02-11
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2003  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*
 *  This file is stolen from GPL'ed DOpE.
 */

#ifndef _THREAD_HH_
#define _THREAD_HH_


#define MUTEX struct mutex_struct

#include <l4/semaphore/semaphore.h>

#define MUTEX_LOCKED 1
#define MUTEX_UNLOCKED 0

MUTEX
{
  l4semaphore_t sem;
  signed char locked_flag;
};

MUTEX *create_mutex (int init);

/*** DESTROY MUTEX ***/
void destroy_mutex (MUTEX * m);
/*** LOCK MUTEX ***/
void mutex_lock (MUTEX * m);
/*** UNLOCK MUTEX ***/
void mutex_unlock (MUTEX * m);
/*** TEST IF MUTEX IS LOCKED ***/
signed char mutex_is_locked (MUTEX * m);

#endif
