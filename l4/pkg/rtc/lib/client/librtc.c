/* $Id$ */
/**
 * \file   rtc/lib/client/librtc.c
 * \brief  client stub
 *
 * \date   09/23/2003
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/rtc/rtc.h>
#include <l4/sys/types.h>
#include <l4/util/rdtsc.h>

#include "rtc-client.h"

static l4_threadid_t server = L4_INVALID_ID;
static l4_uint32_t s_offs_to_systime;
static l4_uint32_t linux_scaler;

/* We need to define this scaler here for use with l4_tsc_to_ns */
l4_uint32_t l4_scaler_tsc_to_ns;

/**
 * A fast and cheap way to calculate without violate the 32-bit range */
static inline l4_uint32_t
muldiv (l4_uint32_t val, l4_uint32_t mul, l4_uint32_t div)
{
  l4_uint32_t dummy;

  asm volatile ("mull %3 ; divl %4\n\t"
               :"=a" (val), "=d" (dummy)
               : "0" (val),  "d" (mul),  "c" (div));
  return val;
}

/**
 * Connect to server and retreive general values */
static int
init_done(void)
{
  if (l4_is_invalid_id(server))
    {
      if (!names_waitfor_name("RTC", &server, 5000))
	{
	  // RTC server NOT FOUND -- don't try again
	  server = L4_NIL_ID;
	  return -L4_EINVAL;
	}

      if (!l4_is_nil_id(server))
	{
	  CORBA_Environment _env = dice_default_environment;

	  if (l4rtc_if_get_offset_call(&server, &s_offs_to_systime, &_env)
	      || _env.major != CORBA_NO_EXCEPTION)
	    return -L4_EINVAL;

	  if (l4rtc_if_get_linux_tsc_scaler_call(&server, &linux_scaler, &_env)
	      || _env.major != CORBA_NO_EXCEPTION)
	    return -L4_EINVAL;

	  l4_scaler_tsc_to_ns = muldiv(linux_scaler, 1000, 1<<5);
	}
    }

  return 0;
}

/**
 * Deliver the numbers of seconds elapsed since 01.01.1970. This value is
 * needed by Linux. */
int
l4rtc_get_seconds_since_1970(l4_uint32_t *seconds)
{
  l4_uint32_t s, ns;

  if (init_done())
    return -L4_EINVAL;

  l4_tsc_to_s_and_ns(l4_rdtsc(), &s, &ns);
  *seconds = s + s_offs_to_systime;
  return 0;
}

/**
 * Deliver the offset between real time and system's uptime in seconds.
 * Some applications want to compute their time in other ways as done
 * in l4rtc_get_seconds_since_1970(). */
int
l4rtc_get_offset_to_realtime(l4_uint32_t *offset)
{
  if (init_done())
    return -L4_EINVAL;

  *offset = s_offs_to_systime;
  return 0;
}

/**
 * Deliver the scaler 2^32 / (tsc clocks per usec). This value is needed by
 * Linux. */
int
l4rtc_get_linux_tsc_scaler(l4_uint32_t *scaler)
{
  if (init_done())
    return -L4_EINVAL;

  *scaler = linux_scaler;
  return 0;
}

