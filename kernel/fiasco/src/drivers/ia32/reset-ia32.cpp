IMPLEMENTATION[ia32]:

#include "io.h"

// reset PC
void __attribute__ ((noreturn))
pc_reset(void)
{
  Io::out8_p(0x80,0x70);
  Io::in8_p (0x71);

  while (Io::in8(0x64) & 0x02)
    ;

  Io::out8_p(0x8F,0x70);
  Io::out8_p(0x00,0x71);

  Io::out8_p(0xFE,0x64);

  for (;;)
    ;
}

