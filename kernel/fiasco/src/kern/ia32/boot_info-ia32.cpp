/* IA32 Specific Boot info */

INTERFACE:

EXTENSION class Boot_info 
{
private:
  static Address  mbi_pa;
  static unsigned flag;
  static unsigned checksum_ro;
  static unsigned checksum_rw;
  static multiboot_info kmbi;
};


IMPLEMENTATION[ia32]:

#include <cassert>
#include <cstring>
#include "checksum.h"
#include "cmdline.h"
#include "linker_syms.h"

// these members needs to be initialized with some
// data to go into the data section and not into bss
Address  Boot_info::mbi_pa      = 125;
unsigned Boot_info::flag        = 3;
unsigned Boot_info::checksum_ro = 15;
unsigned Boot_info::checksum_rw = 16;

// initialized after startup cleaned out the bss

multiboot_info Boot_info::kmbi;



/// \defgroup pre init setup
/**
 * The Boot_info object must be set up with these functions
 * before Boot_info::init() is called!
 * This can be done either in __main, if booted on hardware
 * or in an initializer with a higher priority than BOOT_INFO_INIT_PRIO
 * (e.g MAX_INIT_PRIO) if the kernel runs on software (FIASCO-UX)
 */
//@{

PUBLIC inline static 
void Boot_info::set_flags(unsigned aflags)
{  flag = aflags; }

PUBLIC inline static 
void Boot_info::set_checksum_ro(unsigned ro_cs)
{  checksum_ro = ro_cs; }

PUBLIC inline static 
void Boot_info::set_checksum_rw(unsigned rw_cs)
{  checksum_rw = rw_cs; }
//@}


static inline
void *phys_to_virt(Address addr) // physical to kernel-virtual
{
  return reinterpret_cast<void *>(addr + (Address)&_physmem_1);
}

IMPLEMENT
void
Boot_info::init()
{
  assert(get_flags() == MULTIBOOT_VALID); /* we need to be multiboot-booted */

  kmbi = *(multiboot_info *)(phys_to_virt(mbi_phys()));

  Cmdline::init (kmbi.flags & MULTIBOOT_CMDLINE ?
                 static_cast<char*>(phys_to_virt (kmbi.cmdline)) : "");
}

PUBLIC inline static 
unsigned int Boot_info::get_flags(void)
{
  return flag;
}

PUBLIC inline static 
unsigned Boot_info::get_checksum_ro(void)
{
  return checksum_ro;
}

PUBLIC inline static 
unsigned Boot_info::get_checksum_rw(void)
{
  return checksum_rw;
}

PUBLIC static
void Boot_info::reset_checksum_ro(void)
{
  set_checksum_ro(Checksum::get_checksum_ro());
}

PUBLIC inline static
void Boot_info::set_mbi_phys(Address phys)
{
  mbi_pa = phys;
}

IMPLEMENT inline
Address
Boot_info::mbi_phys(void)
{
  return mbi_pa;
}

IMPLEMENT inline
multiboot_info * const
Boot_info::mbi_virt()
{
  return &kmbi;
}
