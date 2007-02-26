IMPLEMENTATION[{ia32,amd64}-pit_timer]:

#include "irq_alloc.h"
#include "pit.h"
#include "pic.h"

#include <cstdio>

IMPLEMENT
void
Timer::init()
{
  puts("Using the PIT (i8254) on IRQ 0 for scheduling");

  Irq_alloc::lookup(0)->alloc( (Receiver*)-1, false );

  // set up timer interrupt (~ 1ms)
  Pit::init();

  // from now we can save energy in getchar()
  Config::getchar_does_hlt_works_ok = Config::hlt_works_ok;
}

IMPLEMENT inline NEEDS["pic.h"]
void
Timer::acknowledge()
{
  Pic::acknowledge_locked(0);
}

IMPLEMENT inline NEEDS["pic.h"]
void
Timer::enable()
{
  Pic::enable(0);
}

IMPLEMENT inline NEEDS["pic.h"]
void
Timer::disable()
{
  Pic::disable(0);
}

IMPLEMENT inline
void
Timer::update_timer(Unsigned64)
{
  // does nothing in periodic mode
}
