IMPLEMENTATION [arm && pxa]:

#include "timer.h"

void __attribute__ ((noreturn))
  pc_reset(void)
{
  enum {
    OSCR  = Kmem::Timer_map_base + 0x10,
    OWER  = Kmem::Timer_map_base + 0x18,
  };
  Io::write(1, OWER);
  Io::write(0xffffff00, OSCR);

  for (;;)
    ;
}
