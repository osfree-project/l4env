// --------------------------------------------------------------------------
INTERFACE [arm && integrator]:

#include "kmem.h"

EXTENSION class Timer
{
private:
  enum {
    Base = Kmem::Timer_map_base,

    TIMER0_VA_BASE = Base + 0x000,
    TIMER1_VA_BASE = Base + 0x100,
    TIMER2_VA_BASE = Base + 0x200,

    TIMER_LOAD   = 0x00,
    TIMER_VALUE  = 0x04,
    TIMER_CTRL   = 0x08,
    TIMER_INTCLR = 0x0c,

    TIMER_CTRL_IE       = 1 << 5,
    TIMER_CTRL_PERIODIC = 1 << 6,
    TIMER_CTRL_ENABLE   = 1 << 7,
  };
};

// ----------------------------------------------------------------------
IMPLEMENTATION [arm && integrator]:

#include "config.h"
#include "kip.h"
#include "irq_alloc.h"
#include "pic.h"
#include "io.h"

#include <cstdio>

PUBLIC static
void
Timer::watchdog_reboot()
{
}

IMPLEMENT
void Timer::init()
{
  /* Switch all timers off */
  Io::write(0, TIMER0_VA_BASE + TIMER_CTRL);
  Io::write(0, TIMER1_VA_BASE + TIMER_CTRL);
  Io::write(0, TIMER2_VA_BASE + TIMER_CTRL);

  unsigned timer_ctrl = TIMER_CTRL_ENABLE | TIMER_CTRL_PERIODIC;
  unsigned timer_reload = 1000000 / Config::scheduler_granularity;

  Io::write(timer_reload, TIMER1_VA_BASE + TIMER_LOAD);
  Io::write(timer_reload, TIMER1_VA_BASE + TIMER_VALUE);
  Io::write(timer_ctrl | TIMER_CTRL_IE, TIMER1_VA_BASE + TIMER_CTRL);

  Irq_alloc::lookup(Config::Scheduling_irq)->alloc((Receiver*)-1);
}

static inline
Unsigned64
Timer::timer_to_us(Unsigned32 /*cr*/)
{ return 0; }

static inline
Unsigned64
Timer::us_to_timer(Unsigned64 us)
{ (void)us; return 0; }

IMPLEMENT inline NEEDS["io.h", "pic.h", Timer::timer_to_us, <cstdio>]
void Timer::acknowledge()
{
  Io::write(1, TIMER1_VA_BASE + TIMER_INTCLR);

  if (Config::scheduler_one_shot)
    {
      //Kip::k()->clock += timer_to_us(Io::read<Unsigned32>(OSCR));
    }
  else
    {
      Kip::k()->clock += Config::scheduler_granularity;
    }

  Pic::enable_locked(Config::Scheduling_irq);
}

IMPLEMENT inline NEEDS["pic.h"]
void Timer::enable()
{
  Pic::enable(Config::Scheduling_irq);
}

IMPLEMENT inline NEEDS["pic.h"]
void Timer::disable()
{
  Pic::disable(Config::Scheduling_irq);
}

IMPLEMENT inline NEEDS["kip.h", "io.h", Timer::timer_to_us, Timer::us_to_timer]
void
Timer::update_one_shot(Unsigned64 wakeup)
{
  Unsigned32 apic;
  //Kip::k()->clock += timer_to_us(Io::read<Unsigned32>(OSCR));
  Unsigned64 now = Kip::k()->clock;

  if (EXPECT_FALSE (wakeup <= now) )
    // already expired
    apic = 1;
  else
    {
      apic = us_to_timer(wakeup - now);
      if (EXPECT_FALSE(apic > 0x0ffffffff))
	apic = 0x0ffffffff;
      if (EXPECT_FALSE (apic < 1) )
	// timeout too small
	apic = 1;
    }
}

IMPLEMENT inline NEEDS["config.h", "kip.h", "io.h", Timer::timer_to_us]
Unsigned64
Timer::system_clock()
{
  if (Config::scheduler_one_shot)
    //return Kip::k()->clock + timer_to_us(Io::read<Unsigned32>(OSCR));
    return 0;
  else
    return Kip::k()->clock;
}

