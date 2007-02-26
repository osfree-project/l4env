/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/lib/src/process.c
 * \brief  Process Level
 *
 * \date   08/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/** \ingroup mod_common
 * \defgroup mod_proc Process Level Activities
 *
 * This module emulates the process level environment inside the Linux kernel.
 *
 * It provides one task structure (PCB) per worker (L4-)thread. Functions like
 * schedule(), sleep_on() and wake_up() rely on this. Kernel threads are also
 * implemented here.
 *
 * Requirements: (additionally to \ref pg_req)
 *
 * - Linux DDE Memory Management Module
 */
/*****************************************************************************/

/* L4 */
#include <l4/env/errno.h>
#include <l4/thread/thread.h>

#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/sched.h>

/* local */
#include "__config.h"
#include "internal.h"

/** thread data key for "current" data */
static int _key = -L4_ENOKEY;

/** initial task structure */
static struct task_struct _data = INIT_TASK(_data);

/** initialization flag */
static int _initialized = 0;

/** Get pointer to current task structure
 * \ingroup mod_proc
 *
 * This replaces the "current" macro
 *
 * \krishna What about "current" derefences from irqs and softirqs? Are they
 * all eliminated?
 */
struct task_struct * get_current()
{
  void *p = l4thread_data_get_current(_key);
  Assert(p);
  return (struct task_struct *) p;
}

/** Add caller as new process level worker
 * \ingroup mod_proc
 *
 * \return 0 on success; negative error code otherwise
 *
 * This allocates and initializes a new task_struct for the worker thread.
 */
int l4dde_process_add_worker()
{
  int err;
  void *data;

  /* thread data key allocation failed */
  if (_key < 0)
    return _key;

  /* we need a current struct */
  data = vmalloc(sizeof(struct task_struct));
  if (!data)
    return -L4_ENOMEM;
  memcpy(data, &_data, sizeof(struct task_struct));

  if ((err = l4thread_data_set_current(_key, data)))
    return err;

#if DEBUG_PROCESS
  LOG("additional task struct @ %p\n", data);
#endif

  return 0;
}

/** Initalize process module
 * \ingroup mod_proc
 *
 * \return 0 on success; negative error code otherwise
 *
 * Initializes process level emulation environment and adds caller as first and
 * only worker thread. Additional threads can be used after calling
 * l4dde_process_add_worker() in new thread's context.
 */
int l4dde_process_init()
{
  int err;
  void *data;

  if (_initialized)
    return -L4_ESKIPPED;

  /* alloc key for current */
  if ((_key = l4thread_data_allocate_key()) < 0)
    return _key;

  /* we need a current struct */
  data = vmalloc(sizeof(struct task_struct));
  if (!data)
    return -L4_ENOMEM;
  memcpy(data, &_data, sizeof(struct task_struct));

  if ((err = l4thread_data_set_current(_key, data)))
    return err;

#if DEBUG_PROCESS
  LOG("task struct @ %p\n", data);
#endif

  ++_initialized;
  return 0;
}

/** Kernel thread startup helper */
struct kernel_thread_data
{
  int (*fn)(void *);
  void *arg;
};

/** Kernel thread startup helper */
static void __start_kernel_thread(struct kernel_thread_data *data)
{
  int ret;

  if (l4dde_process_add_worker())
    Panic("add_worker() failed");
  if (l4thread_started(NULL))
    Panic("l4thread_started() failed");
  ret = data->fn(data->arg);
  vfree(data);
}



/** Create kernel thread
 * \ingroup mod_proc */
long kernel_thread(int (*fn)(void *), void *arg, unsigned long flags)
{
  int err;
  struct kernel_thread_data *data;

  data = vmalloc(sizeof(struct kernel_thread_data));
  data->fn = fn;
  data->arg = arg;

  err = l4thread_create_long(L4THREAD_INVALID_ID,
                             (l4thread_fn_t) __start_kernel_thread,
                             ".kthread%.2X",
                             L4THREAD_INVALID_SP,
                             L4THREAD_DEFAULT_SIZE,
                             L4THREAD_DEFAULT_PRIO,
                             (void *) data,
                             L4THREAD_CREATE_SYNC);

#if 0
  if ( err < 0 )
    /* XXX Is this the correct value? */
    return -EAGAIN;

  /* XXX take care of pid values ? */
  /* XXX What about err==0 ? */
#endif
  return err;
}
