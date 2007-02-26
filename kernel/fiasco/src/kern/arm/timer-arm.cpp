INTERFACE [arm-{sa1100,pxa}]:

#include "kmem.h"

EXTENSION class Timer
{
private:
  enum {
    Base = Kmem::Timer_map_base,
  };
  // Base = 0x0000000 must be set in ...
  enum {
    OSMR0 = Base + 0x00,
    OSMR1 = Base + 0x04,
    OSMR2 = Base + 0x08,
    OSMR3 = Base + 0x0c,
    OSCR  = Base + 0x10,
    OSSR  = Base + 0x14,
    OWER  = Base + 0x18,
    OIER  = Base + 0x1c,
  };
};

INTERFACE [arm-integrator]:

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

INTERFACE [arm-realview]:

#include "kmem.h"

EXTENSION class Timer
{
private:
  enum {
    SYSTEM_CONTROL = Kmem::System_ctrl_map_base,

    REFCLK = 0,
    TIMCLK = 1,

    TIMER1_ENABLE = 15,
    TIMER2_ENABLE = 17,
    TIMER3_ENABLE = 19,
    TIMER4_ENABLE = 21,

    TIMER_LOAD   = 0x00,
    TIMER_VALUE  = 0x04,
    TIMER_CTRL   = 0x08,
    TIMER_INTCLR = 0x0c,

    LOAD_0 = Kmem::Timer0_map_base + TIMER_LOAD,
    LOAD_1 = Kmem::Timer1_map_base + TIMER_LOAD,
    LOAD_2 = Kmem::Timer2_map_base + TIMER_LOAD,
    LOAD_3 = Kmem::Timer3_map_base + TIMER_LOAD,

    VALUE_0 = Kmem::Timer0_map_base + TIMER_VALUE,
    VALUE_1 = Kmem::Timer1_map_base + TIMER_VALUE,
    VALUE_2 = Kmem::Timer2_map_base + TIMER_VALUE,
    VALUE_3 = Kmem::Timer3_map_base + TIMER_VALUE,

    CTRL_0 = Kmem::Timer0_map_base + TIMER_CTRL,
    CTRL_1 = Kmem::Timer1_map_base + TIMER_CTRL,
    CTRL_2 = Kmem::Timer2_map_base + TIMER_CTRL,
    CTRL_3 = Kmem::Timer3_map_base + TIMER_CTRL,

    INTCLR_0 = Kmem::Timer0_map_base + TIMER_INTCLR,
    INTCLR_1 = Kmem::Timer1_map_base + TIMER_INTCLR,
    INTCLR_2 = Kmem::Timer2_map_base + TIMER_INTCLR,
    INTCLR_3 = Kmem::Timer3_map_base + TIMER_INTCLR,

    INTERVAL = 10000,

    CTRL_IE        = 1 << 5,
    CTRL_PERIODIC  = 1 << 6,
    CTRL_ENABLE    = 1 << 7,
  };
};

// ------------------------------------------------------------------------

IMPLEMENTATION [arm-{sa1100,pxa}]:

#include "config.h"
#include "kip.h"
#include "irq_alloc.h"
#include "pic.h"
#include "io.h"

static unsigned const timer_diff = (36864 * Config::scheduler_granularity)/10000; // 36864MHz*1ms

#include <cstdio>

PUBLIC static
void 
Timer::watchdog_reboot()
{
  Io::write(1, OWER);
  Io::write(0xffffff00, OSCR);
}

IMPLEMENT
void Timer::init()
{
  Io::write(1,          OIER); // enable OSMR0
  Io::write(0,          OWER); // disable Watchdog
  Io::write(timer_diff, OSMR0);
  Io::write(0,          OSCR); // set timer counter to zero
  Io::write(~0U,        OSSR); // clear all status bits

  Irq_alloc::lookup(26)->alloc( (Receiver*)-1, false );
}

static inline
Unsigned64 
Timer::timer_to_us(Unsigned32 cr)
{ return (((Unsigned64)cr) << 14) / 60398; }

static inline
Unsigned64
Timer::us_to_timer(Unsigned64 us)
{ return (us * 60398) >> 14; }

IMPLEMENT inline NEEDS["io.h", "pic.h", Timer::timer_to_us]
void Timer::acknowledge()
{
  if (Config::scheduler_one_shot)
    {
      Kip::k()->clock += timer_to_us(Io::read<Unsigned32>(OSCR));
      //puts("Reset timer");
      Io::write(0, OSCR);
      Io::write(0xffffffff, OSMR0);
    }
  else
    {
      Kip::k()->clock += Config::scheduler_granularity;
      Io::write(0, OSCR);
    }
  Io::write(1, OSSR); // clear all status bits
  Pic::enable_locked(26);  
}

IMPLEMENT inline NEEDS["pic.h"]
void Timer::enable()
{
  Pic::enable(26);
}

IMPLEMENT inline NEEDS["pic.h"]
void Timer::disable()
{
  Pic::disable(26);
}

IMPLEMENT inline
void
Timer::init_system_clock()
{
  Kip::k()->clock = 0;
}

IMPLEMENT inline 
void
Timer::update_system_clock()
{
}

static inline NEEDS["kip.h", "io.h", Timer::timer_to_us, Timer::us_to_timer]
void
Timer::update_one_shot(Unsigned64 wakeup)
{
  Unsigned32 apic;
  Kip::k()->clock += timer_to_us(Io::read<Unsigned32>(OSCR));
  Io::write(0, OSCR);
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

  //printf("%15lld: Set Timer to %lld [%08x]\n", now, wakeup, apic);

  Io::write(apic, OSMR0);
  Io::write(1, OSSR); // clear all status bits
}

IMPLEMENT inline NEEDS["config.h", Timer::update_one_shot]
void
Timer::update_timer(Unsigned64 wakeup)
{
  if (Config::scheduler_one_shot)
    update_one_shot(wakeup);
}

IMPLEMENT inline NEEDS["config.h", "kip.h", "io.h", Timer::timer_to_us]
Unsigned64 
Timer::system_clock()
{
  if (Config::scheduler_one_shot)
    return Kip::k()->clock + timer_to_us(Io::read<Unsigned32>(OSCR));
  else
    return Kip::k()->clock;
}

IMPLEMENTATION [arm-integrator]:

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

  Irq_alloc::lookup(Config::Scheduling_irq)->alloc( (Receiver*)-1, false );
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

IMPLEMENT inline
void
Timer::init_system_clock()
{
  Kip::k()->clock = 0;
}

IMPLEMENT inline
void
Timer::update_system_clock()
{
}

static inline NEEDS["kip.h", "io.h", Timer::timer_to_us, Timer::us_to_timer]
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

IMPLEMENT inline NEEDS["config.h", Timer::update_one_shot]
void
Timer::update_timer(Unsigned64 wakeup)
{
  if (Config::scheduler_one_shot)
    update_one_shot(wakeup);
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

IMPLEMENTATION [arm-realview]:

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

  Mword v;

  v = Io::read<Mword>(SYSTEM_CONTROL);
  v |= TIMCLK << TIMER1_ENABLE;
  v |= TIMCLK << TIMER2_ENABLE;
  v |= TIMCLK << TIMER3_ENABLE;
  v |= TIMCLK << TIMER4_ENABLE;
  Io::write<Mword>(v, SYSTEM_CONTROL);

  // all timers off
  Io::write<Mword>(0, CTRL_0);
  Io::write<Mword>(0, CTRL_1);
  Io::write<Mword>(0, CTRL_2);
  Io::write<Mword>(0, CTRL_3);

  Io::write<Mword>(INTERVAL, LOAD_0);
  Io::write<Mword>(INTERVAL, VALUE_0);
  Io::write<Mword>(CTRL_ENABLE | CTRL_PERIODIC | CTRL_IE, CTRL_0);

  Irq_alloc::lookup(Config::Scheduling_irq)->alloc( (Receiver*)-1, false );
}

static inline
Unsigned64
Timer::timer_to_us(Unsigned32 cr)
{ return 0; }

static inline
Unsigned64
Timer::us_to_timer(Unsigned64 us)
{ (void)us; return 0; }

IMPLEMENT inline NEEDS["config.h", "io.h", "pic.h"]
void Timer::acknowledge()
{
  if (!Config::scheduler_one_shot)
    Kip::k()->clock += Config::scheduler_granularity;

  Io::write<Mword>(0, INTCLR_0);
  Pic::acknowledge_locked(Config::Scheduling_irq);
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

IMPLEMENT inline
void
Timer::init_system_clock()
{
  Kip::k()->clock = 0;
}

IMPLEMENT inline
void
Timer::update_system_clock()
{
}

static inline NEEDS["kip.h", "io.h", Timer::timer_to_us, Timer::us_to_timer]
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

IMPLEMENT inline NEEDS["config.h", Timer::update_one_shot]
void
Timer::update_timer(Unsigned64 wakeup)
{
  if (Config::scheduler_one_shot)
    update_one_shot(wakeup);
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
