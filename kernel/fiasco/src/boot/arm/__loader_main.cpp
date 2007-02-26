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

extern "C" int main(Address s, Address r);

extern "C" 
void __main(Address sigma0, Address root)
{
  atexit(&static_destruction);
  static_construction();
  exit(main(sigma0,root));

}


extern "C" void _exit(int) 
{
  printf("EXIT\n");
  while(1);
}
