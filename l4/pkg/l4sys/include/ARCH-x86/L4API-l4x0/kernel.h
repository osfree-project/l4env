/* 
 * $Id$
 */
#ifndef __L4_KERNEL_H__
#define __L4_KERNEL_H__

#include <l4/sys/types.h>
#include <l4/sys/compiler.h>

typedef struct
{
  l4_umword_t   magic;
  l4_umword_t   version;
  l4_uint8_t    offset_version_strings;
  l4_uint8_t    reserved[7];
  l4_umword_t   init_default_kdebug, default_kdebug_exception;
  l4_umword_t   scheduler_granularity, default_kdebug_end;
  l4_umword_t   sigma0_esp, sigma0_eip;
  l4_low_high_t sigma0_memory;
  l4_umword_t   sigma1_esp, sigma1_eip;
  l4_low_high_t sigma1_memory;
  l4_umword_t   root_esp, root_eip;
  l4_low_high_t root_memory;
  l4_umword_t   l4_config;
  l4_umword_t   reserved2;
  l4_umword_t   kdebug_config;
  l4_umword_t   kdebug_permission;
  l4_low_high_t main_memory;
  l4_low_high_t reserved0, reserved1;
  l4_low_high_t semi_reserved;
  l4_low_high_t dedicated[4];
  volatile l4_uint64_t clock;
  volatile l4_cpu_time_t switch_time;
  l4_uint32_t   frequency_cpu;
  l4_uint32_t   frequency_bus;
  volatile l4_cpu_time_t thread_time;
  l4_uint32_t   sys_ipc;
  l4_uint32_t   sys_id_nearest;
  l4_uint32_t   sys_fpage_unmap;
  l4_uint32_t   sys_thread_switch;
  l4_uint32_t   sys_thread_schedule;
  l4_uint32_t   sys_lthread_ex_regs;
  l4_uint32_t   sys_task_new;

  char  version_strings[512];

  char  sys_calls[256];

  char  pad[288];

  /* ============================================== */
  char  lipc_code[256];

} l4_kernel_info_t;

#define L4_KERNEL_INFO_MAGIC (0x4BE6344CL) /* "L4µK" */

L4_INLINE int l4_kernel_info_version_offset(l4_kernel_info_t *kip);


/*************************************************************************
 * Implementations
 *************************************************************************/

L4_INLINE int
l4_kernel_info_version_offset(l4_kernel_info_t *kip)
{
  return kip->offset_version_strings << 4;
}

#endif
