/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_ts/clientlib/src/free.c
 * \brief  Free a task ID.
 *
 * \date   04/2004
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/util/l4_macros.h>
#include <l4/sys/syscalls.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/generic_ts/generic_ts-client.h>

#include "debug.h"


static int
__do_free_task(const l4_taskid_t *taskid,
               long (*func)(const_CORBA_Object _dice_corba_obj,
                            const l4_taskid_t *taskid,
                            CORBA_Environment *_dice_corba_env))
{
  CORBA_Environment _env = dice_default_environment;
  int error;

  if (!l4ts_connected())
    return -L4_ENOTFOUND;

  if ((error = func(&l4ts_server_id, taskid, &_env)) < 0
      || DICE_HAS_EXCEPTION(&_env))
    {
      LOGd(DEBUG_TASK, "failed (server=" l4util_idfmt", ret=%d, exc %d)",
	   l4util_idstr(l4ts_server_id), error, DICE_EXCEPTION_MAJOR(&_env));
      return error ? error : -L4_EIPC;
    }

  return 0;
}

int
l4ts_free_task(const l4_taskid_t *taskid)
{
  return __do_free_task(taskid, l4_ts_free_call);
}

int
l4ts_free2_task(const l4_taskid_t *taskid)
{
  return __do_free_task(taskid, l4_ts_free2_call);
}
