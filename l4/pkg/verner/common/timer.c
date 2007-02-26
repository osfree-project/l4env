/*
 * \brief   Timer utils for VERNER
 * \date    2004-02-11
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2003  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/*
 *  This file is stolen from GPL'ed DOpE.
 */

#include <l4/util/rdtsc.h>
#include <l4/util/macros.h>
#include <l4/names/libnames.h>
#include <l4/thread/thread.h>

#include "timer.h"

/*************************/
/*** SERVICE FUNCTIONS ***/
/*************************/

/*** RETURN CURRENT SYSTEM TIME COUNTER IN MILLISECONDS ***/
inline unsigned long
get_time_millisec (void)
{
  long long msecs;
  long long ns = l4_tsc_to_ns (l4_rdtsc ());
  msecs = ns / 1000000;
  return msecs & 0xffffffff;
}

/*** RETURN CURRENT SYSTEM TIME COUNTER IN MICROSECONDS ***/
inline unsigned long
get_time_microsec (void)
{
  long long usecs;
  long long ns = l4_tsc_to_ns (l4_rdtsc ());

  usecs = ns / 1000;
  return usecs & 0xffffffff;
}


/*** RETURN CURRENT SYSTEM TIME COUNTER IN NANOSECONDS ***/
inline unsigned long
get_time_nanosec (void)
{
  long long ns = l4_tsc_to_ns (l4_rdtsc ());
  return ns & 0xffffffff;
}


/*** RETURN DIFFERENCE BETWEEN TWO TIMES ***/
inline unsigned long
get_diff (unsigned long time1, unsigned long time2)
{

  /* overflow check */
  if (time1 > time2)
  {
    time1 -= time2;
    return (unsigned long) 0xffffffff - time1;
  }
  return time2 - time1;
}


int
init_timer ()
{

  unsigned long scaler;

  if (!(scaler = l4_calibrate_tsc ()))
    Panic ("l4_calibrate_tsc: fucked up");
  return 1;
}
