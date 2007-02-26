
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

const char *irq0_path = "irq0";

IMPLEMENT FIASCO_INIT
void
Timer::init()
{
  if (Boot_info::irq0_disabled())
    return;

  if (!Pic::setup_irq_prov (Pic::IRQ_TIMER, irq0_path, bootstrap))
    {
      puts ("Problems setting up timer interrupt!");
      exit (1);
    }

  Irq_alloc::lookup (Pic::IRQ_TIMER)->alloc ((Receiver *) -1, false);
}

IMPLEMENT FIASCO_INIT
void
Timer::bootstrap()
{
  close (Boot_info::fd());
  execl (irq0_path, "[I](irq0)", NULL);
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
