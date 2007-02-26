IMPLEMENTATION[arm]:

#include "config.h"
#include "kip.h"
#include "irq_alloc.h"
#include "pic.h"
#include "sa_1100.h"

static unsigned const timer_diff = (36864 * Config::microsec_per_tick)/10000; // 36864MHz*1ms

#include <cstdio>

IMPLEMENT
void Timer::init()
{
  Sa1100::hw_reg(1,          Sa1100::OIER); // enable OSMR0
  Sa1100::hw_reg(0,          Sa1100::OWER); // disable Watchdog
  Sa1100::hw_reg(timer_diff, Sa1100::OSMR0);
  Sa1100::hw_reg(0,          Sa1100::OSCR); // set timer counter to zero
  Sa1100::hw_reg(~0U,        Sa1100::OSSR); // clear all status bits

  Irq_alloc::lookup(26)->alloc( (Receiver*)-1, false );
}

IMPLEMENT inline NEEDS["sa_1100.h","pic.h"]
void Timer::acknowledge()
{
  Sa1100::hw_reg(0, Sa1100::OSCR);
  Sa1100::hw_reg(1, Sa1100::OSSR); // clear all status bits
  Pic::enable_locked(26);  
}

IMPLEMENT //inline NEEDS["pic.h"]
void Timer::enable()
{
  Pic::enable(26);
}

IMPLEMENT //inline NEEDS["pic.h"]
void Timer::disable()
{
  Pic::disable(26);
}

IMPLEMENT inline NEEDS ["config.h", "kip.h"]
void
Timer::update_system_clock()
{
  Kernel_info::kip()->clock += Config::microsec_per_tick;
}
