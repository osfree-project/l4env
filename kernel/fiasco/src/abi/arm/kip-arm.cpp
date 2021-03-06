INTERFACE [arm]:

#include "types.h"

EXTENSION class Kip
{
public:

  Mword magic;
  Mword version;
  Mword offset_version_strings;
  Mword res0; //offset_memory_descs;

  /* 0x10 */
  Mword _res1[4];


  /* the following stuff is undocumented; we assume that the kernel
     info page is located at offset 0x1000 into the L4 kernel boot
     image so that these declarations are consistent with section 2.9
     of the L4 Reference Manual */

  /* 0x20 */
  Mword sigma0_sp, sigma0_ip;
  Mword _res2[2];

  /* 0x30 */
  Mword sigma1_sp, sigma1_ip;
  Mword _res3[2];

  /* 0x40 */
  Mword root_sp,   root_ip;
  Mword _res4[2];

  /* 0x50 */
  Mword l4_config;
  Mword _mem_info;
  Mword kdebug_config;
  Mword kdebug_permission;

  /* 0x60 */
  Mword total_ram;
  Mword _res6[15];

  /* 0xA0 */
  volatile Cpu_time clock;
  Mword _res7[2];

  /* 0xB0 */
  Mword frequency_cpu;
  Mword frequency_bus;
  Mword _res10[2];

  /* 0xC0 */
  Mword irq_desc;
  Mword _res11[7];

  /* 0xE0 */
  Mword user_ptr;
  Mword vhw_offset;
  Unsigned8 vkey_irq;
  char _res8[7];
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include "l4_types.h"

PRIVATE
Address Kip::get_main_memory_size() const
{
  Mem_desc const *m = mem_descs();
  unsigned n = num_mem_descs();
  Address min = ~0UL, max = 0;

  for (; n > 0; n--, m++)
    if (m->type() == Mem_desc::Conventional
        && !m->is_virtual())
      {
	if (min > m->start())
	  min = m->start();
	if (max < m->end())
	  max = m->end();
      }

  return max - min + 1;
}

IMPLEMENT inline
Address Kip::main_memory_high() const
{
  // Static variable but Kip is only used once
  static Address memory_size = get_main_memory_size();

  /* Shouldn't this rather be ram_base + memory_size according
   * to the name? */

  return memory_size;
}

IMPLEMENT
char const *Kip::version_string() const
{
  return reinterpret_cast <char const *> (this) + (offset_version_strings << 4);
}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && debug]:

IMPLEMENT inline
void
Kip::debug_print_syscalls() const
{}

