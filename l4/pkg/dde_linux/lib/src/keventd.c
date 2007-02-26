/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/lib/src/context.c
 * \brief  In-kernel keventd emulation
 *
 * \date   10/05/2005
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 */
#include <l4/thread/thread.h>

#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/sched.h>

/* local */
#include "internal.h"
#include "__config.h"

static DECLARE_TASK_QUEUE(tq_context);
static DECLARE_WAIT_QUEUE_HEAD(context_task_wq);
//static DECLARE_WAIT_QUEUE_HEAD(context_task_done);

int schedule_task(struct tq_struct *task)
{
  int ret;
//  need_keventd(__FUNCTION__);
  ret = queue_task(task, &tq_context);
  wake_up(&context_task_wq);
  return ret;
}

static void dde_keventd_thread(void *dummy)
{
  struct task_struct *curtask;

  if (l4dde_process_add_worker())
    Panic("l4dde_process_add_worker() failed");
  curtask = current;

  DECLARE_WAITQUEUE(wait, curtask);

  /* we are up */
  if (l4thread_started(NULL) < 0)
    Panic("keventd thread startup failed!");

  LOGd(DEBUG_MSG, "dde_keventd_thread "l4util_idfmt" running.",
       l4util_idstr(l4thread_l4_id(l4thread_myself())));

  for (;;)
    {
      set_task_state(curtask, TASK_INTERRUPTIBLE);
      add_wait_queue(&context_task_wq, &wait);
      if (TQ_ACTIVE(tq_context))
        set_task_state(curtask, TASK_RUNNING);
      schedule();
      remove_wait_queue(&context_task_wq, &wait);
      run_task_queue(&tq_context);
//       wake_up(&context_task_done);
    }
}

int l4dde_keventd_init(void)
{
  int err;
  static int _initialized = 0;

  if (_initialized)
    return 0;

  /* create keventd thread */
  err = l4thread_create_long(L4THREAD_INVALID_ID,
                             (l4thread_fn_t) dde_keventd_thread,
                             ".keventd",
                             L4THREAD_INVALID_SP,
                             L4THREAD_DEFAULT_SIZE,
                             L4THREAD_DEFAULT_PRIO,
                             (void *) 0,
                             L4THREAD_CREATE_SYNC);

  if (err < 0)
    return err;

  ++_initialized;
  return 0;
}
