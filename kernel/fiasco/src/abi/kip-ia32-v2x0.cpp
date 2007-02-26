/*
 * IA-32 Kernel-Info Page
 */

INTERFACE:

#include "types.h"

EXTENSION class Kernel_info
{
public:

  /* 00 */
  Mword magic;
  Mword version;
  Unsigned8 offset_version_strings;
  Unsigned8 reserved[3];
  Unsigned8 kip_sys_calls;
  Unsigned8 reserved01[3];

  /* the following stuff is undocumented; we assume that the kernel
     info page is located at offset 0x1000 into the L4 kernel boot
     image so that these declarations are consistent with section 2.9
     of the L4 Reference Manual */

  /* 10 */
  Mword init_default_kdebug, default_kdebug_exception, 
    __unknown, default_kdebug_end;

  /* 20 */
  Mword sigma0_esp, sigma0_eip;
  l4_low_high_t sigma0_memory;

  /* 30 */
  Mword sigma1_esp, sigma1_eip;
  l4_low_high_t sigma1_memory;
  
  /* 40 */
  Mword root_esp, root_eip;
  l4_low_high_t root_memory;

  /* 50 */
  Mword l4_config;
  Mword reserved2;
  Mword kdebug_config;
  Mword kdebug_permission;

  /* 60 */
  l4_low_high_t main_memory;
  l4_low_high_t reserved0;

  /* 70 */
  l4_low_high_t reserved1;
  l4_low_high_t semi_reserved;

  /* 80 */
  l4_low_high_t dedicated[4];

  /* A0 */
  volatile Cpu_time clock;
  Mword unused_4[2];

  /* B0 */
  Mword frequency_cpu;
  Mword frequency_bus;
  Mword unused_5[2];

  /* C0 */
  Mword sys_ipc;
  Mword sys_id_nearest;
  Mword sys_fpage_unmap;
  Mword sys_thread_switch;

  /* D0 */
  Mword sys_thread_schedule;
  Mword sys_lthread_ex_regs;
  Mword sys_task_new;
  Mword unused_6;

  /* E0 */
  char  version_strings[256];
  char  sys_calls[];
};


IMPLEMENTATION[ia32-v2x0]:

#include "l4_types.h"
#include <cstdio>

IMPLEMENT inline
Address Kernel_info::main_memory_high() const
{ 
  return main_memory.high; 
}

IMPLEMENT inline NEEDS ["l4_types.h"]
Mword const Kernel_info::max_threads() const 
{
  return L4_uid::max_threads();
}

