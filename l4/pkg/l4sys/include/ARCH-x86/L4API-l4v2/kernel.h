/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4sys/include/ARCH-x86/L4API-l4v2/kernel.h
 * \brief   Kernel Info Page (KIP)
 * \ingroup api_types
 */
/*****************************************************************************/
#ifndef __L4_KERNEL_H__
#define __L4_KERNEL_H__

#include <l4/sys/types.h>

/**
 * L4 Kernel Info Page
 * \ingroup api_types_kip
 */
typedef struct
{
  l4_umword_t            magic;               /**< Kernel Info Page
					       **  identifier ("L4킟")
					       **/
  l4_umword_t            version;             ///< Kernel version
  l4_uint8_t             offset_version_strings;
#if 0
  l4_uint8_t             reserved[7 + 5 * 16];
#else
  l4_uint8_t             reserved[7];

  /* the following stuff is undocumented; we assume that the kernel
     info page is located at offset 0x1000 into the L4 kernel boot
     image so that these declarations are consistent with section 2.9
     of the L4 Reference Manual */

  /* Kernel debugger */
  l4_umword_t            init_default_kdebug; ///< Kdebug init function
  l4_umword_t            default_kdebug_exception; ///< Kdebug exception handler
  l4_umword_t            __unknown;
  l4_umword_t            default_kdebug_end;

  /* Sigma0 */
  l4_umword_t            sigma0_esp;          ///< Sigma0 start stack pointer
  l4_umword_t            sigma0_eip;          ///< Sigma0 instruction pointer
  l4_low_high_t          sigma0_memory;       ///< Sigma0 code/data area

  /* Sigma1 */
  l4_umword_t            sigma1_esp;          ///< Sigma1 start stack pointer
  l4_umword_t            sigma1_eip;          ///< Sigma1 instruction pointer
  l4_low_high_t          sigma1_memory;       ///< Sigma1 code/data

  /* Root task */
  l4_umword_t            root_esp;            ///< Root task stack pointer
  l4_umword_t            root_eip;            ///< Root task instruction pointer
  l4_low_high_t          root_memory;         ///< Root task code/data

  /* L4 configuration */
  l4_umword_t            l4_config;           /**< L4 kernel configuration
					       **
					       ** Values:
					       **  - bits 0-7: set the number
					       **    of page table entries to
					       **    allocate
					       **  - bits 8-15: set the number
					       **    of mapping nodes.
					       **/
  l4_umword_t            reserved2;
  l4_umword_t            kdebug_config;       /**< Kernel debugger config
					       **
					       **  Values:
					       **  - bits 0-7: set the number
					       **    of pages to allocate for
					       **    the trace buffer
					       **  - bit 8: if set to 1, the
					       **    kernel enters kdebug
					       **    before starting the root
					       **    task
					       **  - bits 16-19: set the port
					       **    speed to use with serial
					       **    line (1..115.2KBd,
					       **    2..57.6KBd, 3..38.4KBd,
					       **    6..19.2KBd, 12..9.6KBD)
					       **  - bits 20-31: set the I/O
					       **    port to use with serial
					       **    line, 0 indicates that no
					       **    serial output should be
					       **    used
					       **/
  l4_umword_t            kdebug_permission;   /**< Kernel debugger permissions
					       **
					       **  Values:
					       **  - bits 0-7: if 0 all tasks
					       **    can enter the kernel
					       **    debugger, otherwise only
					       **    tasks with a number lower
					       **    the set value can enter
					       **    kdebug, other tasks will be
					       **    shut down.
					       **  - bit 8: if set, kdebug may
					       **    display mappings
					       **  - bit 9: if set, kdebug may
					       **    display user registers
					       **  - bit 10: if set, kdebug may
					       **    display user memory
					       **  - bit 11: if set, kdebug may
					       **    modify memory, registers,
					       **    mappings and tcbs
					       **  - bit 12: if set, kdebug may
					       **    read/write I/O ports
					       **  - bit 13: if set, kdebug may
					       **    protocol page faults and
					       **    IPC
					       **/
#endif

  l4_low_high_t          main_memory;         ///< Main memory area
  l4_low_high_t          reserved0;           ///< Reserved memory (kernel code)
  l4_low_high_t          reserved1;           ///< Reserved memory (kernel data)
  l4_low_high_t          semi_reserved;       ///< Reserved memory
  l4_low_high_t          dedicated[4];        ///< Dedicated memory areas

  volatile l4_cpu_time_t clock;               ///< L4 system clock (탎)

  l4_uint32_t            reserved3[2];
  l4_uint32_t            frequency_cpu;       ///< CPU frequency in kHz
  l4_uint32_t            frequency_bus;       ///< Bus frequency
  l4_uint32_t            reserved4[2];

  /* System call entries */
  l4_uint32_t            sys_ipc;             ///< ipc syscall entry
  l4_uint32_t            sys_id_nearest;      ///< id_nearest syscall entry
  l4_uint32_t            sys_fpage_unmap;     ///< fpage_unmap syscall entry
  l4_uint32_t            sys_thread_switch;   ///< thread_switch syscall entry
  l4_uint32_t            sys_thread_schedule; ///< thread_schedule syscall entry
  l4_uint32_t            sys_lthread_ex_regs; /**< sys_lthread_ex_regs
					       **  syscall entry
					       **/
  l4_uint32_t            sys_task_new;        ///< sys_task_new syscall entry

} l4_kernel_info_t;

/**
 * Kernel Info Page identifier ("L4킟")
 * \ingroup api_types_kip
 */
#define L4_KERNEL_INFO_MAGIC (0x4BE6344CL) /* "L4킟" */

#endif
