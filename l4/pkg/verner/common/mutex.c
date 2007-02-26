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

#include "mutex.h"


#include <l4/semaphore/semaphore.h>
#include <stdlib.h> /* malloc, free */

/*********************/

/*** CREATE NEW MUTEX AND SET IT UNLOCKED ***/
MUTEX *
create_mutex (int init)
{
  MUTEX *result;

  result = (MUTEX *) malloc (sizeof (MUTEX));
  if (!result)
  {
    return NULL;
  }

  if (init == MUTEX_LOCKED)
  {
    result->sem = L4SEMAPHORE_LOCKED;
    result->locked_flag = 1;

  }
  else if (init == MUTEX_UNLOCKED)
  {
    result->sem = L4SEMAPHORE_UNLOCKED;
    result->locked_flag = 0;
  }
  else
  {
    result->sem = L4SEMAPHORE_INIT (init);
    result->locked_flag = 1;
  }

  return (MUTEX *) result;
}



/*** DESTROY MUTEX ***/
void
destroy_mutex (MUTEX * m)
{
  if (!m)
    return;
  free (m);
  m = NULL;
}


/*** LOCK MUTEX ***/
void
mutex_lock (MUTEX * m)
{
  if (!m)
    return;
  l4semaphore_down (&m->sem);
  m->locked_flag = 1;
}


/*** UNLOCK MUTEX ***/
void
mutex_unlock (MUTEX * m)
{
  if (!m)
    return;
  l4semaphore_up (&m->sem);
  m->locked_flag = 0;
}


/*** TEST IF MUTEX IS LOCKED ***/
signed char
mutex_is_locked (MUTEX * m)
{
  if (!m)
    return 0;
  return m->locked_flag;
}
