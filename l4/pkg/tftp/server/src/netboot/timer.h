#ifndef __TIMER_H
#define __TIMER_H

#include <l4/util/rdtsc.h>

#define TICKS_PER_MS	1000

extern l4_cpu_time_t timer2_to;

static inline void
load_timer2(unsigned ticks)
{
  timer2_to = l4_rdtsc() + l4_ns_to_tsc(ticks*1000LL);
}

static inline int
timer2_running(void)
{
  return l4_rdtsc() < timer2_to;
}

static inline void
waiton_timer2(unsigned ticks)
{
  load_timer2(ticks);
  while (timer2_running())
    ;
}

#endif

