IMPLEMENTATION[ia32]:

#include "kip.h"
#include "cpu.h"
#include "config.h"

PUBLIC static inline NEEDS ["config.h", "kip.h", "cpu.h"]
void
timer::update_system_clock()
{
  if (Config::kinfo_timer_uses_rdtsc)
    Kernel_info::kip()->clock = Cpu::time_us();
  else
    Kernel_info::kip()->clock += Config::microsec_per_tick;

  do_timeouts();
}
