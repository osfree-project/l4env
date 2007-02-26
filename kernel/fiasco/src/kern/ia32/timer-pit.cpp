IMPLEMENTATION[pit]:

#include "irq_alloc.h"
#include "pit.h"
#include "pic.h"

#include <cstdio>

IMPLEMENT
void Timer::init()
{
  puts("Using the PIT (i8254) on IRQ 0 for scheduling");

  Irq_alloc::lookup(0)->alloc( (Receiver*)-1, false );

  // set up timer interrupt (~ 1ms)
  Pit::init(1000);
}

IMPLEMENT inline NEEDS["pic.h"]
void Timer::acknowledge()
{
  Pic::disable_locked(0);
  Pic::acknowledge_locked(0);
  Pic::enable_locked(0);
}

IMPLEMENT inline NEEDS["pic.h"]
void Timer::enable()
{
  Pic::enable(0);
}

IMPLEMENT inline NEEDS["pic.h"]
void Timer::disable()
{
  Pic::disable(0);
}
