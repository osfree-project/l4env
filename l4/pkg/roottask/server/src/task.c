/**
 * \file    roottask/server/src/task.c
 * \brief   task resource handling
 *
 * \date    05/10/2004
 * \author  Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author  Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/
#include <stdio.h>
#include <string.h>

#include "task.h"
#include "quota.h"
#include "rmgr.h"

static owner_t __task[RMGR_TASK_MAX];


void
task_init(void)
{
  memset(__task, O_RESERVED, RMGR_TASK_MAX);
}

void
task_set(unsigned begin, unsigned end, int state)
{
  int i;

  for (i = begin + 1; i < end; i++)
    __task[i] = state;
}

/* XXX there are two cases were task_alloc can fail XXX */
int
task_alloc(unsigned taskno, owner_t owner, int allow_realloc)
{
  if (taskno >= ELEMENTS(__task))
    return 0;
  if (__task[taskno] == owner && allow_realloc)
    return 1;
  if (__task[taskno] != O_FREE)
      return 0;
  if (! quota_alloc_task(owner, taskno))
      return 0;
  __task[taskno] = owner;
  return 1;
}

int
task_free(unsigned taskno, owner_t owner)
{
  if (taskno >= ELEMENTS(__task))
    return 0;
  if (__task[taskno] != owner && __task[taskno] != O_FREE)
    return 0;

  if (__task[taskno] != O_FREE)
    {
      quota_free_task(owner, taskno);
      __task[taskno] = O_FREE;
    }
  return 1;
}

#ifdef USE_TASKLIB

/** Allocate next free task to owner. */
int
task_next(unsigned owner)
{
  int i;

  if (owner >= ELEMENTS(__task))
    return 0;

  /* Find free task */
  for (i = 0; i < RMGR_TASK_MAX; i++)
    if (__task[i] == O_FREE)
      break;

  /* Out of tasks? */
  if (i == RMGR_TASK_MAX)
    return -1;

  /* Out of quota? */
  if (!quota_alloc_task(owner, i))
    return -1;

  /* Set owner and return task number */
  __task[i] = owner;
  return i;
}


int task_next_explicit(owner_t owner, unsigned long task)
{
  if (owner >= ELEMENTS(__task) || task >= ELEMENTS(__task))
    return 0;

  if (__task[task] != O_FREE)
    return -1;
  else
    __task[task] = owner;
  return task;
}


void
task_free_owned(unsigned owner)
{
  int i;

  for (i = 0; i < RMGR_TASK_MAX; i++)
    task_free(i, owner);
}
#endif

owner_t
task_owner(unsigned taskno)
{
  if (taskno >= ELEMENTS(__task))
    return O_RESERVED;

  return __task[taskno];
}

void
task_dump(void)
{
  int i = 0;

  for (i = 0; i < 200; i++)
    printf("%2x->%2x  ", i, task_owner(i));
}
