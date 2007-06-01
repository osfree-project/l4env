#ifndef __L4__ARCH_ARM__KERNEL_H__
#define __L4__ARCH_ARM__KERNEL_H__

#include <l4/sys/types.h>

/**
 * L4 Kernel Info Page.
 * \ingroup api_types_kip
 */
typedef struct
{
  /* offset 0x00 */
  l4_umword_t            magic;               /**< Kernel Info Page
					       **  identifier ("L4µK").
					       **/
  l4_uint32_t            version;             ///< Kernel version
  l4_uint8_t             offset_version_strings;
  l4_uint8_t             res0[3];
  l4_umword_t            res01;

  /* the following stuff is undocumented; we assume that the kernel
     info page is located at offset 0x1000 into the L4 kernel boot
     image so that these declarations are consistent with section 2.9
     of the L4 Reference Manual */

  /* offset 0x10 */
  l4_umword_t res1[4];

  /* offset 0x20 */
  /* Sigma0 */
  l4_umword_t            sigma0_esp;          ///< Sigma0 start stack pointer
  l4_umword_t            sigma0_eip;          ///< Sigma0 instruction pointer
  l4_umword_t res2[2];

  /* offset 0x30 */
  /* Sigma1 */
  l4_umword_t            sigma1_esp;          ///< Sigma1 start stack pointer
  l4_umword_t            sigma1_eip;          ///< Sigma1 instruction pointer
  l4_umword_t res3[2];

  /* offset 0x40 */
  /* Root task */
  l4_umword_t            root_esp;            ///< Root task stack pointer
  l4_umword_t            root_eip;            ///< Root task instruction pointer
  l4_umword_t res4[2];

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
  l4_umword_t            mem_info;
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
  l4_umword_t            total_ram;
  l4_umword_t            res6[15];

  /* offset 0xA0 */
  volatile l4_cpu_time_t clock;               ///< L4 system clock (µs)
  l4_uint32_t            reserved3[2];

  /* offset 0xB0 */
  l4_uint32_t            frequency_cpu;       ///< CPU frequency in kHz
  l4_uint32_t            frequency_bus;       ///< Bus frequency
  l4_umword_t		 user_ptr;
  l4_uint8_t             vkey_irq;            ///< Number of virtual key interrupt
  l4_uint8_t             reserved4[3];

  /* offset 0xC0 */

} l4_kernel_info_t;

/**
 * Kernel Info Page identifier ("L4µK").
 * \ingroup api_types_kip
 */
#define L4_KERNEL_INFO_MAGIC (0x4BE6344CL) /* "L4µK" */

L4_INLINE
int
l4_kernel_info_version_offset(l4_kernel_info_t *kip);



/*************************************************************************
 * Implementations
 *************************************************************************/


L4_INLINE
int
l4_kernel_info_version_offset(l4_kernel_info_t *kip)
{
  return kip->offset_version_strings << 4;
}

#endif /* __L4__ARCH_ARM__KERNEL_H__ */
