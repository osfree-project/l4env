IMPLEMENTATION [arm-sa1100]:

#include "io.h"
#include "k_sa1100.h"

void __attribute__ ((noreturn))
pc_reset(void)
{
  Io::write( (Mword)Sa1100::RSRR_SWR, (Address)Sa1100::RSRR );
  for (;;);
}

IMPLEMENTATION [arm-pxa]:

#include "timer.h"

void __attribute__ ((noreturn))
pc_reset(void)
{
  Timer::watchdog_reboot();
  for (;;);
}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm-isg]:

void __attribute__ ((noreturn))
pc_reset(void)
{
  for (;;);
}
