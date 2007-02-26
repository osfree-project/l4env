/* 
 * $Id$
 */
#ifndef __L4_KERNEL_H__ 
#define __L4_KERNEL_H__ 

#include <l4/sys/types.h>

typedef struct 
{
  l4_umword_t magic;
  l4_umword_t version;
  l4_uint8_t offset_version_strings;
#if 0
  l4_uint8_t reserved[7 + 5 * 16];
#else
  l4_uint8_t reserved[7];

  /* the following stuff is undocumented; we assume that the kernel
     info page is located at offset 0x1000 into the L4 kernel boot
     image so that these declarations are consistent with section 2.9
     of the L4 Reference Manual */
  l4_umword_t init_default_kdebug, default_kdebug_exception, 
    __unknown, default_kdebug_end;
  l4_umword_t sigma0_esp, sigma0_eip;
  l4_low_high_t sigma0_memory;
  l4_umword_t sigma1_esp, sigma1_eip;
  l4_low_high_t sigma1_memory;
  l4_umword_t root_esp, root_eip;
  l4_low_high_t root_memory;
  l4_umword_t l4_config;
  l4_umword_t reserved2;
  l4_umword_t kdebug_config;
  l4_umword_t kdebug_permission;
#endif
  l4_low_high_t main_memory;
  l4_low_high_t reserved0, reserved1;
  l4_low_high_t semi_reserved;
  l4_low_high_t dedicated[4];
  volatile unsigned long long clock;
  l4_uint32_t reserved3[2];
  l4_uint32_t frequency_cpu;
  l4_uint32_t frequency_bus;
  l4_uint32_t reserved4[2];
  l4_uint32_t sys_ipc;
  l4_uint32_t sys_id_nearest;
  l4_uint32_t sys_fpage_unmap;
  l4_uint32_t sys_thread_switch;
  l4_uint32_t sys_thread_schedule;
  l4_uint32_t sys_lthread_ex_regs;
  l4_uint32_t sys_task_new;

  
} l4_kernel_info_t;

#define L4_KERNEL_INFO_MAGIC (0x4BE6344CL) /* "L4µK" */

#endif
