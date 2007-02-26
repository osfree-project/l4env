/* IA32 specific */
INTERFACE:
#include "types.h"

IMPLEMENTATION:
#include "boot_info.h"
#include "checksum.h"
#include "initcalls.h"

#include <cstdlib>
#include <cstdio>
#include <panic.h>
#include <initfini.h>

extern "C" int main(void);

extern "C" 
void __main(Address mbi_phys, unsigned int aflag,
	    unsigned achecksum_ro, unsigned achecksum_rw) FIASCO_INIT;

extern "C" 
void __main(Address mbi_phys, unsigned int aflag,
	    unsigned achecksum_ro, unsigned achecksum_rw)
{
  if (Checksum::get_checksum_ro() != achecksum_ro)
    panic("read-only (text) checksum does not match");

  if (Checksum::get_checksum_rw() != achecksum_rw)
    panic("read-write (data) checksum does not match");

  
  /* set global to be used in the constructors */
  Boot_info::set_mbi_phys(mbi_phys);
  Boot_info::set_flags(aflag);
  Boot_info::set_checksum_ro( achecksum_ro );

  atexit(&static_destruction);  
  static_construction();

  exit(main());

}
