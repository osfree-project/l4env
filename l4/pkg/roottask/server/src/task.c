/**
 * \file	roottask/server/src/task.c
 * \brief	task resource handling
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
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
task_alloc(unsigned taskno, owner_t owner)
{
  if (__task[taskno] == owner)
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
task_next(owner_t owner)
{
  int i;

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

void
task_free_owned(owner_t owner)
{
  int i;

  for (i = 0; i < RMGR_TASK_MAX; i++)
    task_free(i, owner);
}
#endif

owner_t
task_owner(unsigned taskno)
{
  return __task[taskno];
}

void
task_dump(void)
{
  int i = 0;

  for (i = 0; i < 200; i++)
    printf("%2x->%2x  ", i, task_owner(i));
}
