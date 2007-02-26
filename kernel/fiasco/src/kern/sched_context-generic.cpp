IMPLEMENTATION[generic]:

#include "config.h"
#include "types.h"

IMPLEMENT inline NEEDS["config.h","types.h"]
Unsigned64 Sched_context::get_total_cputime() const
{
  return (Unsigned64)_total_ticks * Config::microsec_per_tick;
}
