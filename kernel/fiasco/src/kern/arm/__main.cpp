/* ARM specific */

INTERFACE:
#include "types.h"



IMPLEMENTATION:
#include <cstdlib>
#include <cstdio>
#include <initfini.h>
#include "boot_info.h"


extern "C" int main(void);

void console_init();

extern "C" 
void __main( void* kip )
{
  Boot_info::set_kip((Kernel_info*)kip);

  atexit(&static_destruction);
  static_construction();

  exit(main());
}


