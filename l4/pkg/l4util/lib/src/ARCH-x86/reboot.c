#include <l4/util/port_io.h>
#include <l4/util/cpu.h>

#include "../reboot_arch.h"

void
l4util_reboot_arch(void)
{
  while (l4util_in8(0x64) & 0x02)
    l4util_iodelay();
  l4util_out8(0x60, 0x64);
  l4util_iodelay();

  while (l4util_in8(0x64) & 0x02)
    l4util_iodelay();
  l4util_out8(0x04, 0x60);
  l4util_iodelay();

  while (l4util_in8(0x64) & 0x02)
    l4util_iodelay();
  l4util_out8(0xFE, 0x64);
  l4util_iodelay();

  for (;;)
    l4util_cpu_pause();
}
