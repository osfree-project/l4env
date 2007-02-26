IMPLEMENTATION[arm]:

#include "config.h"
#include "types.h"

IMPLEMENT
Unsigned64 sched_t::get_total_cputime() const
{
  return (Unsigned64)_total_ticks * Config::microsec_per_tick;
}
