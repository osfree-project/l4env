/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_ts/clientlib/src/exit.c
 * \brief  Terminate the caller.
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
#include <l4/util/util.h>
#include <l4/util/l4_macros.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/generic_ts/generic_ts-client.h>

#include "debug.h"

int
l4ts_exit(void)
{
  CORBA_Environment _env = dice_default_environment;
  int error;

  if (!l4ts_connected())
    {
      LOG("Task server not found -- sleeping forever");
      l4_sleep_forever();
    }

  for (;;)
    {
      if ((error = l4_ts_exit_call(&l4ts_server_id, &_env)) < 0
     	  || _env.major != CORBA_NO_EXCEPTION)
	{
	  LOGd(DEBUG_TASK, "failed (server=" l4util_idfmt
			   ", ret=%d, exc %d) -- sleeping",
	   l4util_idstr(l4ts_server_id), error, _env.major);
	  l4_sleep_forever();
	}

      LOGd(DEBUG_EXIT, "return -- trying again");
    }
}
