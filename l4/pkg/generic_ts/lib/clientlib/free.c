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
#include <l4/generic_ts/generic_ts.h>
#include <l4/generic_ts/generic_ts-client.h>

#include "debug.h"

int
l4ts_free_task(const l4_taskid_t *taskid)
{
  CORBA_Environment _env = dice_default_environment;
  int error;

  if (!l4ts_connected())
    return -L4_ENOTFOUND;

  if ((error = l4_ts_free_call(&l4ts_server_id, taskid, &_env)) < 0
      || _env.major != CORBA_NO_EXCEPTION)
    {
      LOGd(DEBUG_TASK, "failed (server=" l4util_idfmt", ret=%d, exc %d)",
	   l4util_idstr(l4ts_server_id), error, _env.major);
      return error ? error : -L4_EIPC;
    }

  return 0;
}
