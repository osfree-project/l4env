/*
 * IA-32 Kernel-Info Page
 */

INTERFACE [ux]:

#include "vhw.h"

INTERFACE [ia32,ux]:

#include "types.h"

EXTENSION class Kip
{
public:

  /* 00 */
  Mword      magic;
  Mword      version;
  Unsigned8  offset_version_strings;
  Unsigned8  fill0[3];
  Unsigned8  kip_sys_calls;
  Unsigned8  fill1[3];

  /* the following stuff is undocumented; we assume that the kernel
     info page is located at offset 0x1000 into the L4 kernel boot
     image so that these declarations are consistent with section 2.9
     of the L4 Reference Manual */

  /* 10 */
  Mword      init_default_kdebug;
  Mword      default_kdebug_exception;
  Mword      sched_granularity;
  Mword      default_kdebug_end;

  /* 20 */
  Mword      sigma0_sp, sigma0_ip;
  Mword      _res2[2];

  /* 30 */
  Mword      sigma1_sp, sigma1_ip;
  Mword      _res3[2];

  /* 40 */
  Mword      root_sp, root_ip;
  Mword	     _res4[2];

  /* 50 */
  Mword      l4_config;
  Mword      _mem_info;
  Mword      kdebug_config;
  Mword      kdebug_permission;

  /* 60 */
  Mword total_ram;
  Mword _res6[15];

  /* A0 */
  volatile Cpu_time clock;
  volatile Cpu_time switch_time;

  /* B0 */
  Mword frequency_cpu;
  Mword frequency_bus;
  volatile Cpu_time thread_time;

  /* C0 */
  Mword      sys_ipc;
  Mword      sys_id_nearest;
  Mword      sys_fpage_unmap;
  Mword      sys_thread_switch;

  /* D0 */
  Mword      sys_thread_schedule;
  Mword      sys_lthread_ex_regs;
  Mword      sys_task_new;
  Mword      sys_privctrl;

  Mword user_ptr;

  Mword vhw_offset;

  char __pad[8];
};

// =======================================================================
IMPLEMENTATION [ia32,ux]:

#include "l4_types.h"
#include <cstdio>

IMPLEMENT inline
Address Kip::main_memory_high() const
{
  static unsigned long mem_h = 0;
  if (!mem_h)
    {
      Mem_desc const *m = mem_descs();
      unsigned n = num_mem_descs();
      for (; n > 0; n--, m++)
	if (m->type() == Mem_desc::Conventional
	    && !m->is_virtual())
	  {
	    if (mem_h < m->end())
	      mem_h = m->end();
	  }
    }

  return mem_h;
}

IMPLEMENT
char const *Kip::version_string() const
{
  return reinterpret_cast <char const *> (this) + (offset_version_strings << 4);
}

IMPLEMENTATION [ux]:

PUBLIC
Vhw_descriptor *
Kip::vhw() const
{ 
  return reinterpret_cast<Vhw_descriptor*>(((unsigned long)this) + vhw_offset); }
