/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4io/server/src/jiffies.c
 * \brief  L4Env l4io I/O Server jiffies Thread
 *
 * \date   05/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/kernel.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sigma0/kip.h>
#include <l4/util/util.h>
#include <l4/rmgr/librmgr.h>
#include <l4/env/errno.h>
#include <l4/l4rm/l4rm.h>
#include <l4/thread/thread.h>

/* local includes */
#include "io.h"
#include "jiffies.h"
#include "__config.h"
#include "__macros.h"


/*
 * vars
 */
static unsigned long long volatile *kclock; /**< kernel clock reference
                                             * \ingroup grp_misc */

/** Map kernel info page.
 *
 * \return 0 on success, negative error code otherwise
 *
 * Map L4 kernel info page (L4 kernel timer as time base).
 */
static int __map_kernel_info_page(void)
{
  l4_kernel_info_t *kip;

  if (!(kip = l4sigma0_kip_map(L4_INVALID_ID)))
    {
      LOGdL(DEBUG_ERRORS, "error getting kernel info page!");
      return -L4_ENOTFOUND;
    }

  kclock = &kip->clock;

  return 0;
}

/** Jiffies thread loop.
 * \ingroup grp_misc
 *
 * \param  data         dummy data pointer (unused)
 *
 * This thread implements the LINUX jiffies counter.
 * It sleeps and tries to correct the period using the L4 kernel clock.
 *
 * XXX test for (wait < 0) to increment jiffies by 2 or more if JIFFIE_PERIOD
 * is too short
 */
static void jiffies_thread(void *data)
{
  int wait;
  int next_tick = 0;

  /* I'm up */
  l4thread_started(NULL);

  next_tick = *kclock + 5 * IOJIFFIES_PERIOD; /* it's just the first tick */

  /* jiffie loop */
  for (;;)
    {
      wait = next_tick - *kclock;

      while (wait > 0)
        {
          l4thread_usleep((l4_uint32_t) wait);
          wait = next_tick - *kclock;
        }

      io_info.jiffies++;
      io_info.xtime.tv_sec = io_info.jiffies / IOJIFFIES_HZ;
      next_tick += IOJIFFIES_PERIOD;
    }

  /* that should never happen */
  Panic("left jiffies loop!\n");
}

/** Jiffies thread initialization.
 * \ingroup grp_misc
 *
 * \return 0 on success, negative error code otherwise
 *
 * Initialize jiffie counter thread.
 */
int io_jiffies_init()
{
  int error;
  l4thread_t jiffies_tid;

  /* map kernel info (time base) */
  if ((error = __map_kernel_info_page()))
    return error;

  /* create jiffies thread */
  jiffies_tid = l4thread_create_named(jiffies_thread,
				      ".jiffies",
				      0, L4THREAD_CREATE_SYNC);

  if (jiffies_tid <= 0)
    {
      LOGdL(DEBUG_ERRORS, "create jiffies thread (%d)", jiffies_tid);
      return jiffies_tid;
    }

  return 0;
}
