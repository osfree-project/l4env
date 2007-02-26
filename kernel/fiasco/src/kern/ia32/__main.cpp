INTERFACE:
#include "types.h"

IMPLEMENTATION:
#include "boot_info.h"
#include "initcalls.h"

#include <cstdlib>
#include <cstdio>
#include <initfini.h>

extern "C" int main(void);

extern "C" FIASCO_FASTCALL
void
__main(Address mbi_phys, unsigned aflag, unsigned checksum_ro)
{
  /* set global to be used in the constructors */
  Boot_info::set_mbi_phys(mbi_phys);
  Boot_info::set_flags(aflag);
  Boot_info::set_checksum_ro(checksum_ro);
  Boot_info::init();

  atexit(&static_destruction);  
  static_construction();

  exit(main());
}
