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

IMPLEMENTATION [arm-integrator]:

#include "io.h"
#include "kmem.h"

void __attribute__ ((noreturn))
pc_reset(void)
{
  enum
    {
      HDR_CTRL_OFFSET = Kmem::Integrator_map_base + 0xc,
    };

  Io::write(1 << 3, HDR_CTRL_OFFSET);

  for (;;);
}

IMPLEMENTATION [arm-realview]:

#include "io.h"
#include "kmem.h"

void __attribute__ ((noreturn))
pc_reset(void)
{
  enum
    {
      LOCK  = Kmem::System_regs_map_base + 0x20,
      RESET = Kmem::System_regs_map_base + 0x40,
    };

  Io::write(0xa05f, LOCK);  // unlock for reset
  Io::write(0x100,  RESET); // reset

  for (;;);
}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm-isg]:

void __attribute__ ((noreturn))
pc_reset(void)
{
  for (;;);
}
