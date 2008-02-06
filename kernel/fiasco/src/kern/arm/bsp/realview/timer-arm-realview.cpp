// --------------------------------------------------------------------------
INTERFACE [arm && realview]:

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

    INTERVAL = 1000,

    CTRL_IE        = 1 << 5,
    CTRL_PERIODIC  = 1 << 6,
    CTRL_ENABLE    = 1 << 7,
  };
};

// -----------------------------------------------------------------------
IMPLEMENTATION [arm && realview]:

#include "config.h"
#include "kip.h"
#include "irq_alloc.h"
#include "pic.h"
#include "io.h"

#include <cstdio>

IMPLEMENT
void Timer::init()
{
  Mword v;

  v = Io::read<Mword>(SYSTEM_CONTROL);
  v |= TIMCLK << TIMER1_ENABLE;
  Io::write<Mword>(v, SYSTEM_CONTROL);

  // all timers off
  Io::write<Mword>(0, CTRL_0);
  Io::write<Mword>(0, CTRL_1);
  Io::write<Mword>(0, CTRL_2);
  Io::write<Mword>(0, CTRL_3);

  Io::write<Mword>(INTERVAL, LOAD_0);
  Io::write<Mword>(INTERVAL, VALUE_0);
  Io::write<Mword>(CTRL_ENABLE | CTRL_PERIODIC | CTRL_IE, CTRL_0);

  Irq_alloc::lookup(Config::Scheduling_irq)->alloc( (Receiver*)-1);
}

static inline
Unsigned64
Timer::timer_to_us(Unsigned32 /*cr*/)
{ return 0; }

static inline
Unsigned64
Timer::us_to_timer(Unsigned64 us)
{ (void)us; return 0; }

IMPLEMENT inline NEEDS["config.h", "io.h", "pic.h"]
void Timer::acknowledge()
{
  // XXX: there's a update_system_clock function !?!?!?!
  //if (!Config::scheduler_one_shot)
  //  Kip::k()->clock += Config::scheduler_granularity;

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

IMPLEMENT inline NEEDS["config.h", "kip.h"]
Unsigned64
Timer::system_clock()
{
  if (Config::scheduler_one_shot)
    return 0;
  else
    return Kip::k()->clock;
}
