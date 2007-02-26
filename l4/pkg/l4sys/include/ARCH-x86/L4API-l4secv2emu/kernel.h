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

typedef struct {
  l4_addr_t start; ///< start address
  l4_addr_t end;   ///< end address
} l4_region_t;

/**
 * L4 Kernel Info Page.
 * \ingroup api_types_kip
 */
typedef struct
{
  /* offset 0x00 */
  l4_uint32_t            magic;               /**< Kernel Info Page
					       **  identifier ("L4킟").
					       **/
  l4_uint32_t            version;             ///< Kernel version
  l4_uint8_t             offset_version_strings;
  l4_uint8_t             fill0[3];
  l4_uint8_t             kip_sys_calls;
  l4_uint8_t             fill1[3];

  /* the following stuff is undocumented; we assume that the kernel
     info page is located at offset 0x1000 into the L4 kernel boot
     image so that these declarations are consistent with section 2.9
     of the L4 Reference Manual */

  /* offset 0x10 */
  /* Kernel debugger */
  l4_umword_t            init_default_kdebug; ///< Kdebug init function
  l4_umword_t            default_kdebug_exception; ///< Kdebug exception handler
  l4_umword_t            scheduler_granularity; ///< for rounding timeslices
  l4_umword_t            default_kdebug_end;

  /* offset 0x20 */
  /* Sigma0 */
  l4_umword_t            sigma0_esp;          ///< Sigma0 start stack pointer
  l4_umword_t            sigma0_eip;          ///< Sigma0 instruction pointer
  l4_region_t	         sigma0_memory;       ///< Sigma0 code/data area

  /* offset 0x30 */
  /* Sigma1 */
  l4_umword_t            sigma1_esp;          ///< Sigma1 start stack pointer
  l4_umword_t            sigma1_eip;          ///< Sigma1 instruction pointer
  l4_region_t	         sigma1_memory;       ///< Sigma1 code/data

  /* offset 0x40 */
  /* Root task */
  l4_umword_t            root_esp;            ///< Root task stack pointer
  l4_umword_t            root_eip;            ///< Root task instruction pointer
  l4_region_t	         root_memory;         ///< Root task code/data

  /* offset 0x50 */
  /* L4 configuration */
  l4_umword_t            l4_config;           /**< L4 kernel configuration.
					       **
					       ** Values:
					       **  - bits 0-7: set the number
					       **    of page table entries to
					       **    allocate
					       **  - bits 8-15: set the number
					       **    of mapping nodes.
					       **/
  l4_umword_t            reserved2;
  l4_umword_t            kdebug_config;       /**< Kernel debugger config.
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
  l4_umword_t            kdebug_permission;   /**< Kernel debugger permissions.
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

  /* offset 0x60 */
  l4_region_t          main_memory;         ///< Main memory area
  l4_region_t          reserved0;           ///< Reserved memory (kernel code)
  l4_region_t          reserved1;           ///< Reserved memory (kernel data)
  l4_region_t          semi_reserved;       ///< Reserved memory
  l4_region_t          dedicated[4];        ///< Dedicated memory areas

  /* offset 0xA0 */
  volatile l4_cpu_time_t clock;               ///< L4 system clock (탎)
  volatile l4_cpu_time_t switch_time;         /**< timestamp of last l4 thread
                                               **  switch (cycles)
                                               **  - only valid if
                                               **    FINE_GRAINED_CPU_TIME is
                                               **    available
                                               **/

  /* offset 0xB0 */
  l4_uint32_t            frequency_cpu;       ///< CPU frequency in kHz
  l4_uint32_t            frequency_bus;       ///< Bus frequency
  volatile l4_cpu_time_t thread_time;         /**< accumulated thread time for
                                               ** currently running thread at
                                               ** last l4 thread switch (in
                                               ** cycles)
                                               **  - only valid if
                                               **    FINE_GRAINED_CPU_TIME is
                                               **    available
                                               **/

  /* offset 0xC0 */
  /* System call entries */
  l4_umword_t            sys_ipc;             ///< ipc syscall entry
  l4_umword_t            sys_id_nearest;      ///< id_nearest syscall entry
  l4_umword_t            sys_fpage_unmap;     ///< fpage_unmap syscall entry
  l4_umword_t            sys_thread_switch;   ///< thread_switch syscall entry
  l4_umword_t            sys_thread_schedule; ///< thread_schedule syscall entry
  l4_umword_t            sys_lthread_ex_regs; /**< sys_lthread_ex_regs
					       **  syscall entry.
					       **/
  l4_umword_t            sys_task_new;        ///< sys_task_new syscall entry.
  l4_umword_t            sys_privctrl;

  /* offset 0xE0 */
  char                   version_strings[512];

  /* offset 0x2E0 */
  char                   sys_calls[256];

  /* offset 0x3E0 */
  char                   pad[288];

  /* ============================================== */
  /* offset 0x500 */
  char                   lipc_code[256];

} l4_kernel_info_t;

/**
 * Kernel Info Page identifier ("L4킟").
 * \ingroup api_types_kip
 */
#define L4_KERNEL_INFO_MAGIC (0x4BE6344CL) /* "L4킟" */

/**
 * \brief Return offset in bytes of version_strings relative to the KIP base.
 * \ingroup api_types_kip
 *
 * \param kip	Pointer to the kernel into page (KIP).
 *
 * \return offset of version_strings relative to the KIP base address, in
 *         bytes.
 */
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
