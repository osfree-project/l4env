/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4io/examples/dummy/main.c
 * \brief  L4Env I/O client example (dumb dummy)
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
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/l4rm/l4rm.h>
#include <l4/thread/thread.h>

#include <l4/util/rdtsc.h>  /* read time-stamp counter */
#include <l4/util/macros.h>

#include <l4/generic_io/libio.h>

#include <stdlib.h>

char LOG_tag[9] = "io_dummy";

/* exported by libio */
extern unsigned long jiffies;
extern unsigned long HZ;

#define MY_INFO_ADDR 0

/* default info page mapping mode */
#ifndef MY_INFO_ADDR
#define MY_INFO_ADDR 0
#endif

#if (MY_INFO_ADDR == 0)
  l4io_info_t *io_info_addr = NULL;
#else
  l4io_info_t *io_info_addr = -1;
#endif

/* use symbol "jiffies" for measurement */
static void measure_jiffies(unsigned int num)
{
  unsigned long new;

  LOG_Enter("HZ = %lu jiffies = %lu @ %p num = %u",
            HZ, jiffies, &jiffies, num);

  l4_calibrate_tsc();

  for (; num; num--)
    {
      l4_cpu_time_t stamp0 = 0;
      l4_cpu_time_t diff;

      stamp0 = l4_rdtsc();
      /* wait HZ jiffies (1 s) */
      new = jiffies + HZ;
      while (jiffies < new) l4thread_usleep(900*(new-jiffies));
      diff = l4_rdtsc();

      diff -= stamp0;

      LOG("period = %lu jiffies (%u ms) ... xtime = {%ld, %ld}",
          HZ, ((l4_uint32_t) l4_tsc_to_ns(diff)) / 1000000,
          io_info_addr->xtime.tv_sec, io_info_addr->xtime.tv_usec);
    }
}

int main(void)
{
  int error;
  l4io_drv_t drv = L4IO_DRV_INVALID;

  LOG("Hello World! io_dummy is up ...");

  if ((error = l4io_init(&io_info_addr, drv)))
    {
      LOG_Error("initalizing libio: %d (%s)", error, l4env_strerror(-error));
      exit(1);
    }

  LOG("libio was initialized.");
  if (io_info_addr->omega0)
    LOG("L4IO is running an omega0.");
  else
    LOG("L4IO doesn't handle Interrupts.");

#if (MY_INFO_ADDR != -1)
  measure_jiffies(100);
#endif

  exit(0);

  /* prevent silly warnings about unused symbols */
  measure_jiffies(100);
}
