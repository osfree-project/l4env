/*
 * IA-32 Kernel-Info Page
 */

INTERFACE [ia32,ux]:

#include "types.h"

EXTENSION class Kip
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
  sched_granularity, default_kdebug_end;

  /* 20 */
  Mword sigma0_sp, sigma0_ip;
  l4_low_high_t sigma0_memory;

  /* 30 */
  Mword sigma1_sp, sigma1_ip;
  l4_low_high_t sigma1_memory;
  
  /* 40 */
  Mword root_sp, root_ip;
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
  volatile Cpu_time switch_time;

  /* B0 */
  Mword frequency_cpu;
  Mword frequency_bus;
  volatile Cpu_time thread_time;

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
  char  version_strings[512];

  /* 2E0 */
  char  sys_calls[256];

  /* 3E0 */
  char  pad[288];

  /* ======================================== */

  /* 0x500,
   *   Here resides the LIPC code. If the offset changes, then application
   *   LIPC binding needs to be adjusted.
   */
  char  lipc_code[256];
};

// =======================================================================
IMPLEMENTATION [ia32,ux]:

#include "l4_types.h"
#include <cstdio>

IMPLEMENT inline
Address Kip::main_memory_high() const
{ 
  return main_memory.high; 
}

IMPLEMENT
char const *Kip::version_string() const
{
  return reinterpret_cast <char const *> (this) + (offset_version_strings << 4);
}
