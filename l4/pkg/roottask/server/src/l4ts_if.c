/* l4ts interface for roottask */
#include "task.h"
#include "pager.h"
#include <l4/task/generic_ts-server.h>

#include <stdio.h>

long
l4_ts_allocate_component(CORBA_Object _dice_corba_obj,
                         l4_taskid_t *taskid,
                         CORBA_Server_Environment *_dice_corba_env)
{
  int num = task_next(_dice_corba_obj->id.task);
  l4_threadid_t n;

  reset_pagefault(num);

  n          = *_dice_corba_obj;
  n.id.task  = num;
  ++n.id.version_low;

  n = l4_task_new(n, (unsigned)_dice_corba_obj->raw, 0, 0, L4_NIL_ID);
  if (l4_is_nil_id(n))
    {
      printf("error setting chief of task %d to %d\n", num,
             _dice_corba_obj->id.task);
      task_free(num, _dice_corba_obj->id.task);
      return 1; /* failed */
    }

  *taskid = n;

  return 0;
}


long
l4_ts_free_component(CORBA_Object _dice_corba_obj,
                     const l4_taskid_t *taskid,
                     CORBA_Server_Environment *_dice_corba_env)
{
  if (!task_free(taskid->id.task, _dice_corba_obj->id.task))
    return -1;

  return 0;
}


static int kill_task(l4_taskid_t task)
{
  l4_threadid_t t;

  t = l4_task_new(task, (unsigned)l4_myself().raw, 0, 0, L4_NIL_ID);

  if (l4_is_invalid_id(t))
    return -1;

  /* Now free all tasks that were owned by killed one. */
  task_free_owned(task.id.task);
  return 0;
}

long
l4_ts_exit_component(CORBA_Object _dice_corba_obj,
                     short *_dice_reply,
                     CORBA_Server_Environment *_dice_corba_env)
{
  return kill_task(*_dice_corba_obj);
}

void
l4_ts_dump_component(CORBA_Object _dice_corba_obj,
                     CORBA_Server_Environment *_dice_corba_env)
{
  printf("ROOT: %s unimplemented.", __func__);
}

void dice_server_error(l4_msgdope_t d, CORBA_Server_Environment *e)
{}
