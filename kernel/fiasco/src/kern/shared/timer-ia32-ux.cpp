IMPLEMENTATION[ia32,ux]:

#include "cpu.h"
#include "config.h"
#include "kip.h"

IMPLEMENT inline NEEDS ["config.h", "cpu.h", "kip.h"]
void
Timer::init_system_clock()
{
  if (Config::kinfo_timer_uses_rdtsc)
    Kip::k()->clock = Cpu::time_us();
  else
    Kip::k()->clock = 0;
}

IMPLEMENT inline NEEDS ["kip.h"]
Unsigned64
Timer::system_clock()
{
  if (Config::kinfo_timer_uses_rdtsc)
    Kip::k()->clock = Cpu::time_us();

  return Kip::k()->clock;
}

IMPLEMENT inline NEEDS ["config.h", "cpu.h", "kip.h"]
void
Timer::update_system_clock()
{
  if (Config::kinfo_timer_uses_rdtsc)
    Kip::k()->clock = Cpu::time_us();
  else
    Kip::k()->clock += Config::scheduler_granularity;
}

