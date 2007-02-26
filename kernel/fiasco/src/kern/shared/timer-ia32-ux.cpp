IMPLEMENTATION[ia32-ux]:

#include "cpu.h"
#include "config.h"
#include "kip.h"

IMPLEMENT inline NEEDS ["config.h", "cpu.h", "kip.h"]
void
Timer::update_system_clock()
{
  if (Config::kinfo_timer_uses_rdtsc)
    Kernel_info::kip()->clock = Cpu::time_us();
  else
    Kernel_info::kip()->clock += Config::microsec_per_tick;
}
