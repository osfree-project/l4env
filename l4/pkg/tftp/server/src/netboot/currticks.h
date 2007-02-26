#ifndef __CURRTICKS_H
#define __CURRTICKS_H

#include <l4/util/rdtsc.h>

static inline unsigned long
currticks (void)
{
  long ticks;
  long long ns = l4_tsc_to_ns (l4_rdtsc ());
  __asm__ __volatile__
	("					\n\t"
	 "divl	%%esi				\n\t"
	:"=a" (ticks)
	:"A" (ns),
	 "S" (1000000000 / TICKS_PER_SEC)
	);
	
  return ticks;
}

#endif /* __CURRTICKS_H */
