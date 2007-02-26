/* IA32 Specific Boot info */

INTERFACE:

#include <flux/x86/multiboot.h>

EXTENSION class Boot_info 
{
private:
  static vm_offset_t mbi_pa;
  static unsigned flag;
  static unsigned checksum_ro;
  static unsigned checksum_rw;
};


IMPLEMENTATION[ia32]:

#include <cassert>
#include <cstring>
#include "checksum.h"
#include "linker_syms.h"

// these members needs to be initialized with some
// data to go into the data section and not into bss
vm_offset_t Boot_info::mbi_pa = 125;
unsigned int Boot_info::flag = 3;
unsigned Boot_info::checksum_ro = 15;
unsigned Boot_info::checksum_rw = 16;

// initialized after startup cleaned out the bss

static multiboot_info kmbi;



/// \defgroup pre init setup
/**
 * The Boot_info object must be set up with these functions
 * before Boot_info::init() is called!
 * This can be done either in __main, if booted on hardware
 * or in an initializer with a higher priority than BOOT_INFO_INIT_PRIO
 * (e.g MAX_INIT_PRIO) if the kernel runs on software (FIASCO-UX)
 */
//@{

PUBLIC static 
void Boot_info::set_flags(unsigned aflags)
{  flag = aflags; }

PUBLIC static 
void Boot_info::set_checksum_ro(unsigned ro_cs)
{  checksum_ro = ro_cs; }

PUBLIC static 
void Boot_info::set_checksum_rw(unsigned rw_cs)
{  checksum_rw = rw_cs; }
//@}


static
void *phys_to_virt(vm_offset_t addr) // physical to kernel-virtual
{
  return reinterpret_cast<void *>(addr + (vm_offset_t)&_physmem_1);
}

IMPLEMENT
void
Boot_info::init()
{
  assert(get_flags() == MULTIBOOT_VALID); /* we need to be multiboot-booted */

  kmbi = *(multiboot_info *)(phys_to_virt(mbi_phys()));
  if (kmbi.flags & MULTIBOOT_CMDLINE)
    {
      strncpy(_cmdline, static_cast<char*>(phys_to_virt(kmbi.cmdline)),
	      sizeof(_cmdline));
      _cmdline[sizeof(_cmdline) - 1] = 0;
    }
  else
    _cmdline[0] = 0;
}

PUBLIC static 
unsigned int Boot_info::get_flags(void)
{
  return flag;
}

PUBLIC static 
unsigned Boot_info::get_checksum_ro(void)
{
  return checksum_ro;
}

PUBLIC static 
unsigned Boot_info::get_checksum_rw(void)
{
  return checksum_rw;
}

PUBLIC static 
void Boot_info::set_mbi_phys(vm_offset_t phys)
{
  mbi_pa = phys;
}

PUBLIC static 
vm_offset_t Boot_info::mbi_phys(void)
{
  return mbi_pa;
}

PUBLIC static
multiboot_info const *Boot_info::mbi_virt()
{
  return &kmbi;
}
