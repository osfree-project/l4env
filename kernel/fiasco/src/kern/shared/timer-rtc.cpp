IMPLEMENTATION[{ia32,amd64}-rtc_timer]:

#include "irq_alloc.h"
#include "rtc.h"
#include "pic.h"
#include "pit.h"

#include <cstdio>

IMPLEMENT
void
Timer::init()
{
#ifdef CONFIG_SLOW_RTC
  puts("Using the RTC on IRQ 8 (64Hz) for scheduling");
#else
  puts("Using the RTC on IRQ 8 (1kHz) for scheduling");
#endif
  
  Irq_alloc::lookup(8)->alloc( (Receiver*)-1, false );

  // set up timer interrupt (~ 1ms)
  Rtc::init();

  // make sure that PIT does pull its interrupt line
  Pit::done();

  // from now we can save energy in getchar()
  Config::getchar_does_hlt_works_ok = Config::hlt_works_ok;
}

IMPLEMENT inline NEEDS["rtc.h","pic.h"]
void
Timer::acknowledge()
{
  // periodic scheduling is triggered by irq 8 connected with RTC
  Pic::disable_locked(8);  
  Rtc::ack_reset();
  Pic::enable_locked(8);
}

IMPLEMENT inline NEEDS["pic.h"]
void
Timer::enable()
{
  Pic::enable(8);
  Pic::enable(2); // cascade
}

IMPLEMENT inline NEEDS["pic.h"]
void
Timer::disable()
{
  Pic::disable(8);
}

IMPLEMENT inline
void
Timer::update_timer(Unsigned64)
{
  // does nothing in periodic mode
}
