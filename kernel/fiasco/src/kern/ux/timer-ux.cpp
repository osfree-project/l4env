
INTERFACE:

EXTENSION class Timer
{
private:
  static void bootstrap();
};

IMPLEMENTATION[ux]:

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include "boot_info.h"
#include "initcalls.h"
#include "irq_alloc.h"
#include "pic.h"

IMPLEMENT FIASCO_INIT
void
Timer::init()
{
  if (Boot_info::irq0_disabled())
    return;

  if (!Pic::setup_irq_prov (Pic::IRQ_TIMER, Boot_info::irq0_path(), bootstrap))
    {
      puts ("Problems setting up timer interrupt!");
      exit (1);
    }

  Irq_alloc::lookup (Pic::IRQ_TIMER)->alloc ((Receiver *) -1);
}

IMPLEMENT FIASCO_INIT
void
Timer::bootstrap()
{
  close (Boot_info::fd());
  execl (Boot_info::irq0_path(), "[I](irq0)", NULL);
}

IMPLEMENT inline
void
Timer::acknowledge()
{}

IMPLEMENT inline NEEDS ["pic.h"]
void
Timer::enable()
{
  if (Boot_info::irq0_disabled())
    return;

  Pic::enable (Pic::IRQ_TIMER);
}

IMPLEMENT inline NEEDS ["pic.h"]
void
Timer::disable()
{
  Pic::disable (Pic::IRQ_TIMER);
}

IMPLEMENT inline
void
Timer::update_timer(Unsigned64)
{
  // does nothing in periodic mode
}
