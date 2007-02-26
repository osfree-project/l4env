IMPLEMENTATION[arm]:

#include "kip.h"
#include "irq_alloc.h"

IMPLEMENT
void Timer::init()
{

#warning MUST allocate IRQ
  //Irq_alloc::lookup(8)->alloc( (Receiver*)-1, false );

  // set up timer interrupt (~ 1ms)
  
  init_done = true;

}

IMPLEMENT inline //NEEDS["rtc.h","pic.h"]
void Timer::acknowledge()
{
#warning MUST ack timer IRQ
  // periodic scheduling is triggered by irq 8 connected with RTC
  //  Pic::disable_locked(8);  
  //  Rtc::ack_reset();
  //  Pic::enable_locked(8);
}

IMPLEMENT inline //NEEDS["pic.h"]
void Timer::enable()
{
#warning MUST enable timer IRQ
  //  Pic::enable(8);
  //  Pic::enable(2); // cascade
}

IMPLEMENT inline //NEEDS["pic.h"]
void Timer::disable()
{
#warning MUST disable timer IRQ
  //  Pic::disable(8);
}

PUBLIC static inline NEEDS ["kip.h"]
void
timer::update_system_clock()
{
  Kernel_info::kip()->clock += Config::microsec_per_tick;
  do_timeouts();
}
