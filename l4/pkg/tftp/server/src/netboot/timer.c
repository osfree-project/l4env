#include <l4/util/rdtsc.h>
#include "etherboot.h"
#include "timer.h"

static l4_cpu_time_t timer2_to;

int
currticks (void)
{
  l4_uint32_t ticks, dummy;
  l4_uint64_t ns = l4_tsc_to_ns (l4_rdtsc ());
  asm volatile ("divl %3" 
		: "=a"(ticks) , "=d"(dummy)
		: "A" (ns), "rm"(1000000000/TICKS_PER_SEC));
  return ticks;
}

void
load_timer2 (unsigned ticks)
{
  timer2_to = l4_rdtsc() + l4_ns_to_tsc (ticks*1000LL);
}

int
timer2_running (void)
{
  return l4_rdtsc() < timer2_to;
}

void
waiton_timer2 (unsigned ticks)
{
  load_timer2 (ticks);
  while (timer2_running ())
    asm (".byte 0xf3, 0x90 #pause");
}
