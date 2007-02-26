IMPLEMENTATION[ia32-ux]:

#include "config.h"
#include "cpu.h"
#include "types.h"

IMPLEMENT
Unsigned64
Sched_context::get_total_cputime() const
{
  if (Config::fine_grained_cputime)
    return Cpu::tsc_to_us (_total_cputime);
  else
    return (Unsigned64)_total_ticks * Config::microsec_per_tick;
}
