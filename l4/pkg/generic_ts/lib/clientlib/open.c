/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_ts/clientlib/src/open.c
 * \brief  Allocate/Create task
 *
 * \date   04/2004
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/env/errno.h>
#include <l4/util/l4_macros.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/generic_ts/generic_ts-client.h>

#include "debug.h"

int
l4ts_allocate_task(l4_taskid_t *taskid)
{
  CORBA_Environment _env = dice_default_environment;
  int error;

  if (!l4ts_connected())
    return -L4_ENOTFOUND;

  if ((error = l4_ts_allocate_call(&l4ts_server_id, taskid, &_env)) < 0
      || DICE_HAS_EXCEPTION(&_env))
    {
      LOGd(DEBUG_TASK, "failed (server=" l4util_idfmt", ret=%d, exc %d)",
           l4util_idstr(l4ts_server_id), error, DICE_EXCEPTION_MAJOR(&_env));
      return error ? error : -L4_EIPC;
    }

  return 0;
}

int
l4ts_allocate_task2(l4_taskid_t *taskid)
{
  CORBA_Environment _env = dice_default_environment;
  int error;

  if (!l4ts_connected())
    return -L4_ENOTFOUND;

  if ((error = l4_ts_allocate2_call(&l4ts_server_id, taskid, &_env)) < 0
      || DICE_HAS_EXCEPTION(&_env))
    {
      LOGd(DEBUG_TASK, "failed (server=" l4util_idfmt", ret=%d, exc %d)",
           l4util_idstr(l4ts_server_id), error, DICE_EXCEPTION_MAJOR(&_env));
      return error ? error : -L4_EIPC;
    }

  return 0;
}

static int
__do_create(l4_taskid_t *taskid, l4_addr_t entry, l4_addr_t stack,
            l4_uint32_t mcp, const l4_taskid_t *pager,
	    const l4_taskid_t *caphandler, l4_int32_t prio,
            const char *resname, l4_uint32_t flags)
{
  CORBA_Environment _env = dice_default_environment;
  int error;

  if (!l4ts_connected())
    return -L4_ENOTFOUND;

  while ((error = l4_ts_create_call(&l4ts_server_id, taskid, entry, stack, mcp,
          pager, caphandler, prio, resname, flags, &_env)) < 0
      || DICE_HAS_EXCEPTION(&_env))
    {
      if (error == -L4_EUSED)
        /* the task is in terminating state, try it again */
        l4_sleep(10);
      else
      {
        LOGd(DEBUG_TASK, "failed (server=" l4util_idfmt", ret=%d, exc %d)",
            l4util_idstr(l4ts_server_id), error, DICE_EXCEPTION_MAJOR(&_env));
        return error ? error : -L4_EIPC;
      }
    }

  return 0;
}

int
l4ts_create_task(l4_taskid_t *taskid, l4_addr_t entry, l4_addr_t stack,
                 l4_uint32_t mcp, const l4_taskid_t *pager, l4_int32_t prio,
		 const char *resname, l4_uint32_t flags)
{
  l4_taskid_t invalidcaphandler = L4_INVALID_ID;
  return __do_create(taskid, entry, stack, mcp, pager, &invalidcaphandler,
                     prio, resname, flags);
}

int
l4ts_create_task2(l4_taskid_t *taskid, l4_addr_t entry, l4_addr_t stack,
                  l4_uint32_t mcp, const l4_taskid_t *pager,
		  const l4_taskid_t *caphandler, l4_int32_t prio,
		  const char *resname, l4_uint32_t flags)
{
  return __do_create(taskid, entry, stack, mcp, pager, caphandler,
                     prio, resname, flags);
}
