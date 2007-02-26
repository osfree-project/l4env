/* ARM specific */

INTERFACE:
#include "types.h"



IMPLEMENTATION:
//#include "boot_info.h"

#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <initfini.h>

#include "uart.h"

extern "C" int main(void);

extern "C" 
void __main(vm_offset_t mbi_pa, unsigned int aflag,
	    unsigned achecksum_ro, unsigned achecksum_rw)
{
  atexit(&static_destruction);
  static_construction();
  printf("mbi_pa=%08x\n",mbi_pa);
  exit(main());

}


extern "C" void _exit(int) 
{
  printf("EXIT\n");
  while(1);
}
