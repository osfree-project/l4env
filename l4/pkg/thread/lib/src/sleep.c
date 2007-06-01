/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/src/sleep.c
 * \brief  Thread sleep.
 *
 * \date   12/28/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * \todo Synchronize with L4 kernel timer (requires abstraction of the
 *       kernel clock in l4env).
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/util/bitops.h>
#include <l4/util/macros.h>
#include <l4/util/util.h>

/* library includes */
#include <l4/thread/thread.h>
#include "__debug.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Sleep.
 *
 * \param  t             Timeout (microseconds)
 *
 * Do sleep.
 *
 * \todo Restart sleep if IPC canceled.
 */
/*****************************************************************************/ 
static void
__do_sleep(l4_uint32_t t)
{
  l4_timeout_t to;
  int error;

  if (t == (l4_uint32_t)-1)
    to = L4_IPC_NEVER;
  else
    {
      l4_timeout_s rcv = l4util_micros2l4to(t);

      if (rcv.t == L4_IPC_TIMEOUT_0.t)
	return;

      to = l4_timeout(L4_IPC_TIMEOUT_NEVER, rcv);
    }

  /* do wait */
  error = l4_ipc_sleep(to);

  if (error != L4_IPC_RETIMEOUT)
    LOG_Error("l4thread: sleep canceled!");
}

/*****************************************************************************
 *** l4thread user API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Sleep.
 *
 * \param  t             Time (milliseconds)
 *
 * Sleep for \t milliseconds.
 */
/*****************************************************************************/
void
l4thread_sleep(l4_uint32_t t)
{
  /* sleep */
  if (t == (l4_uint32_t)-1)
    __do_sleep(-1);
  else
    __do_sleep(t * 1000);
}

/*****************************************************************************/
/**
 * \brief  Sleep.
 *
 * \param  t             Time (microseconds).
 *
 * Sleep for \t microseconds.
 *
 * \note Although the L4 timeout is specified in microseconds, the actual
 *       timer resolution is about one millisecond. If we really need
 *       microsecond timers, we must implement them differently.
 * \todo Implement microsecond timers (if we really need them).
 */
/*****************************************************************************/
void
l4thread_usleep(l4_uint32_t t)
{
  /* sleep */
  __do_sleep(t);
}

/*****************************************************************************/
/**
 * \brief  Sleep forever
 */
/*****************************************************************************/
void
l4thread_sleep_forever(void)
{
  /* sleep */
  __do_sleep(-1);
}
