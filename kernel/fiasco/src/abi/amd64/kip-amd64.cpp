/*
 * IA-32 Kernel-Info Page
 */

INTERFACE [amd64]:

#include "types.h"

EXTENSION class Kip
{
public:

  /* 00 */
  Mword      magic;
  Mword      version;
  Unsigned8  offset_version_strings;
  Unsigned8  fill2[7];
  Unsigned8  kip_sys_calls;
  Unsigned8  fill3[7];

  /* the following stuff is undocumented; we assume that the kernel
     info page is located at offset 0x1000 into the L4 kernel boot
     image so that these declarations are consistent with section 2.9
     of the L4 Reference Manual */

  /* 20 */
  Mword      init_default_kdebug;
  Mword      default_kdebug_exception;
  Mword      sched_granularity;
  Mword      default_kdebug_end;

  /* 40 */
  Mword      sigma0_sp, sigma0_ip;
  Mword	     res2[2];

  /* 60 */
  Mword      sigma1_sp, sigma1_ip;
  Mword	     res3[2];
  
  /* 80 */
  Mword      root_sp, root_ip;
  Mword	     res4[2];

  /* A0 */
  Mword      l4_config;
  Mword      _mem_info;
  Mword      kdebug_config;
  Mword      kdebug_permission;

  /* C0 */
  Mword      total_ram;
  Mword	     res5[15];

  /* 140 */
  volatile Cpu_time clock;
  //Unsigned8  fill4[8];
  volatile Cpu_time switch_time;
  //Unsigned8  fill5[8];

  /* 160 */
  Mword      frequency_cpu;
  Mword      frequency_bus;
  volatile Cpu_time thread_time;
  //Unsigned8  fill6[8];

  /* 180 */
  Mword      sys_ipc;
  Mword      sys_id_nearest;
  Mword      sys_fpage_unmap;
  Mword      sys_thread_switch;

  /* 1A0 */
  Mword      sys_thread_schedule;
  Mword      sys_lthread_ex_regs;
  Mword      sys_task_new;
  Mword      sys_privctrl;

  /* 1C0 */
  Mword      user_ptr;
  Mword      vhw_offset;
  Unsigned8  vkey_irq;
  char       __pad[7];
};


IMPLEMENTATION [amd64]:

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

