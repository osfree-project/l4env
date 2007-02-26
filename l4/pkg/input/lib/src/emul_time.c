/* $Id$ */
/*****************************************************************************/
/**
 * \file   input/lib/src/emul_time.c
 * \brief  L4INPUT: Linux time emulation
 *
 * \date   11/20/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 */
#include <stdio.h>
#include <l4/util/rdtsc.h>     /* XXX x86 specific */
#include <l4/thread/thread.h>

/* Linux */
#include <asm/delay.h>

#include "internal.h"

/* Can be initialized with the value taken from l4io. */
unsigned long l4input_hz = 100;

extern unsigned long HZ __attribute__ ((weak));
extern unsigned long jiffies;

/* JIFFIES EMULATION */

static void __jiffies_thread(void *ignore)
{
  l4thread_started(NULL);

  for (;;)
    {
      l4thread_sleep(1000/l4input_hz);
      jiffies++;
    }
}

/* JIFFIES EMULATION INITIALIZATION */
void l4input_internal_jiffies_init(void)
{
  if (&HZ == 0 /* libio not linked */||
      HZ == 0  /* l4io_info page not mapped*/)
    {
      printf("L4INPUT: libio not linked/initialized "
	     "-- updating jiffies myself\n");
      l4thread_create_long(L4THREAD_INVALID_ID,
			   (l4thread_fn_t) __jiffies_thread,
			   ".jiffies",
			   L4THREAD_INVALID_SP,
			   L4THREAD_DEFAULT_SIZE,
			   L4THREAD_DEFAULT_PRIO,
			   NULL,
			   L4THREAD_CREATE_SYNC);
    }
  else
    l4input_hz = HZ;
}

/* UDELAY */
void udelay(unsigned long usecs)
{
  l4_busy_wait_us(usecs);
}
