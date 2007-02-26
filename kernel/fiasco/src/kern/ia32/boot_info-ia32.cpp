/* IA32 Specific Boot info */

INTERFACE:

EXTENSION class Boot_info 
{
private:
  static Address  _mbi_pa;
  static unsigned _flag;
  static unsigned _checksum_ro;
  static unsigned _checksum_rw;
  static Multiboot_info _kmbi;
};


IMPLEMENTATION[ia32]:

#include <cassert>
#include <cstring>
#include <cstdlib>
#include "checksum.h"
#include "cmdline.h"
#include "mem_layout.h"

// these members needs to be initialized with some
// data to go into the data section and not into bss
Address  Boot_info::_mbi_pa        = 125;
unsigned Boot_info::_flag          = 3;
unsigned Boot_info::_checksum_ro   = 15;
unsigned Boot_info::_checksum_rw   = 16;

// initialized after startup cleaned out the bss

Multiboot_info Boot_info::_kmbi;


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
{  _flag = aflags; }

PUBLIC inline static 
void Boot_info::set_checksum_ro(unsigned ro_cs)
{  _checksum_ro = ro_cs; }

PUBLIC inline static 
void Boot_info::set_checksum_rw(unsigned rw_cs)
{  _checksum_rw = rw_cs; }
//@}


IMPLEMENT
void
Boot_info::init()
{
  // multiboot info is know to reside in the first 4MB
  _kmbi = * Mem_layout::boot_data((Multiboot_info *)mbi_phys());
  Cmdline::init (_kmbi.flags & Multiboot_info::Cmdline 
      ? Mem_layout::boot_data(reinterpret_cast<char*>(_kmbi.cmdline))
      : "");
}

PUBLIC inline static 
unsigned
Boot_info::get_flags(void)
{
  return _flag;
}

PUBLIC inline static 
unsigned
Boot_info::get_checksum_ro(void)
{
  return _checksum_ro;
}

PUBLIC inline static 
unsigned
Boot_info::get_checksum_rw(void)
{
  return _checksum_rw;
}

PUBLIC static
void
Boot_info::reset_checksum_ro(void)
{
  set_checksum_ro(Checksum::get_checksum_ro());
}

PUBLIC inline static
void
Boot_info::set_mbi_phys(Address phys)
{
  _mbi_pa = phys;
}

IMPLEMENT inline
Address
Boot_info::mbi_phys(void)
{
  return _mbi_pa;
}

IMPLEMENT inline
Multiboot_info * const
Boot_info::mbi_virt()
{
  return &_kmbi;
}

PUBLIC static
unsigned long
Boot_info::kmemsize()
{
  const char *c;

  return (  (c = strstr(Cmdline::cmdline(), " -kmemsize="))
	  ||(c = strstr(Cmdline::cmdline(), " -kmemsize ")))
    ? strtol(c+11, 0, 0) << 20
    : 0;
}
