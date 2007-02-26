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

#ifndef _TIMER_HH_
#define _TIMER_HH_

/*
 * \brief	DOpE timer module
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 */

#include <l4/util/kip.h>
#include <l4/util/rdtsc.h>
#include <l4/util/thread_time.h>


/*** RETURN TIME IN NANOSEC ***/
inline unsigned long get_time_nanosec (void);

/*** RETURN TIME IN MICROSEC ***/
inline unsigned long get_time_microsec (void);

/*** RETURN TIME IN MILLISEC ***/
inline unsigned long get_time_millisec (void);

/*** RETURN DIFFERENCE BETWEEN TWO TIMES ***/
inline unsigned long get_diff (unsigned long time1, unsigned long time2);

/*** RETURN THREAD TIME IN MICROSEC ***/
static inline l4_uint64_t get_thread_time_microsec (void)
{
  return l4_tsc_to_us(l4util_thread_time(l4util_kip()));
}

int init_timer (void);

#endif
