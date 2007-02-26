/* Defines for routines to implement a low-overhead timer for drivers */

#ifndef	TIMER_H
#define TIMER_H

#include <l4/util/rdtsc.h>
#include <l4/util/util.h>

/* Timers tick over at this rate */
#define CLOCK_TICK_RATE	1193180U
#define	TICKS_PER_MS	(CLOCK_TICK_RATE/1000)

/* Ticks must be between 0 and 65535 (0 == 65536)
   because it is a 16 bit counter */
extern void load_timer2(unsigned int ticks);
extern inline int timer2_running(void);
extern void waiton_timer2(unsigned int ticks);
extern void __load_timer2(unsigned int ticks);

extern void setup_timers(void);
extern void ndelay(unsigned int nsecs);

static inline void
udelay(unsigned usecs)
{
  return l4_busy_wait_us(usecs);
}

static inline void
mdelay(unsigned msecs)
{
  return l4_sleep(msecs);
}

#endif	/* TIMER_H */
