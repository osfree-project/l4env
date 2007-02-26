#include <l4/sys/kdebug.h>
#include "../reboot_arch.h"

void
l4util_reboot_arch(void)
{
  enter_kdebug("*#^");
  
  for (;;)
    ;        
}
