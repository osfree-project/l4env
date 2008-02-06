IMPLEMENTATION [arm && realview]:

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
  Io::write(Io::read<Mword>(RESET) | 1,  RESET); // reset

  for (;;)
    ;
}
