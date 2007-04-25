/* $Id: */

/**
 * \file   l4io/server/src/mtrr.c
 * \brief  L4Env l4io I/O Server MTRR management
 *
 * \date   05/05/2006
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* MTRR (memory type range registers) set attributes for regions of physical
 * memory. */

#include <string.h>
#include <stdio.h>
#include <l4/sys/types.h>
#include <l4/sys/kdebug.h>
#include <l4/util/cpu.h>
#include <l4/util/l4_macros.h>
#include <l4/log/l4log.h>
#include <l4/env/errno.h>

#include "mtrr.h"

static int         num_mtrr;
static int         available;
static l4_uint64_t size_or_mask  = 0xffffffff00000000ULL;
static l4_uint64_t size_and_mask = 0x00000000fffff000ULL;

/**
 * Read machine-specific register (maybe a candidate for l4/util/cpu.h)
 *
 * \param  reg  number of MSR
 * \return      data
 *
 * \warning     Exception 13 (general protection) is raised if MSR does
 *              not exist.
 */
static inline l4_uint64_t
rdmsr(int reg)
{
  l4_uint32_t low, high;
  asm volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(reg));
  return ((l4_uint64_t)high << 32) | low;
}

/**
 * Write machine-specific register (maybe a candidate for l4/util/cpu.h)
 *
 * \param  reg   number of MSR
 * \param  value data to write (64 bit)
 *
 * \warning      Exception 13 (general protection) is raised if MSR does
 *               not exist or if invalid data should be written.
 */
static inline void
wrmsr(int reg, l4_uint64_t value)
{
  asm volatile ("wrmsr" : : "a"((l4_uint32_t)value),
			    "d"((l4_uint32_t)(value >> 32)),
			    "c"(reg));
}

/**
 * Read variable range MTRR.
 *
 * \param  num   number of MTRR
 * \retval addr  start of region
 * \retval size  size of memory region
 * \retval type  memory attribute
 * \return 0     on success
 */
static int
mtrr_get(int num, l4_addr_t *addr, l4_size_t *size, int *type)
{
  l4_uint64_t mask = rdmsr(0x201 + 2*num);
  l4_uint64_t base;

  if (num > num_mtrr)
    return -L4_EINVAL;

  if ((mask & 0x800) == 0)
    {
      /* disabled */
      *size = 0;
      return 0;
    }

  base = rdmsr(0x200 + 2*num);

  *size = -((size_or_mask | mask) & L4_PAGEMASK);
  *addr = base & L4_PAGEMASK;
  *type = base & 0xff;
  return 0;
}

static const char *
mtrr_mem_type(int type)
{
  switch (type)
    {
    case MTRR_UC: return "UC";
    case MTRR_WC: return "WC";
    case MTRR_WT: return "WT";
    case MTRR_WP: return "WP";
    case MTRR_WB: return "WB";
    default:      return "??";
    }
}

/**
 * Ensure a specific memory attribute for a memory region.
 *
 * \param addr   start of memory region
 * \param size   size of memory region
 * \param type   memory attribute
 * \return 0     on success
 *
 * From Intel reference manual (10.11.4.1, MTRR Precedences):
 *
 * The processor attempts to match the physical address with a memory type set
 * by the variable-range MTRRs:
 *  a. If one variable memory range matches, the processor uses the memory
 *     type stored in the IA32_MTRR_PHYSBASEn register for that range.
 *  b. If two or more variable memory ranges match and the memory types are
 *     identical, then that memory type is used.
 *  c. If two or more variable memory ranges match and one of the memory types
 *     is UC, the UC memory type used.
 *  d. If two or more variable memory ranges match and the memory types are WT
 *     and WB, the WT memory type is used.
 *  e. For overlaps not defined by the above rules, processor behavior is
 *     undefined.
 */
int
mtrr_set(l4_addr_t addr, l4_size_t size, int type)
{
  int i, unused = -1;
  l4_uint64_t base;
  l4_uint64_t mask;

  if (!available)
    return -L4_EINVAL;

  /* first check if new region would overlap with an old region */
  for (i=0; i<num_mtrr; i++)
    {
      l4_addr_t mtrr_addr;
      l4_size_t mtrr_size;
      int       mtrr_type;
      if (0 == mtrr_get(i, &mtrr_addr, &mtrr_size, &mtrr_type) && mtrr_size)
	{
	  /* Found active MTRR. Check if it overlaps the new region. */
	  if (mtrr_addr < addr+size && mtrr_addr+mtrr_size > addr)
	    {
	      if (mtrr_type == type)
		continue; /* rule b */
	      if (mtrr_type == MTRR_UC)
	      if (mtrr_type == MTRR_UC)
		{
		  /* rule c */
		  LOG_printf("Cannot ensure memory type %s from "
		             l4_addr_fmt"-"l4_addr_fmt" (overlaps "
			     l4_addr_fmt"-"l4_addr_fmt" type %s)\n",
			     mtrr_mem_type(type), addr, addr+size,
			     mtrr_addr, mtrr_addr+mtrr_size,
			     mtrr_mem_type(mtrr_type));
		  return -L4_EINVAL;
		}
	      if ((mtrr_type == MTRR_WB && type == MTRR_WT) ||
		  (mtrr_type == MTRR_WT && type == MTRR_WB))
		continue; /* rule d */
	      /* rule e */
	      LOG_printf("Cannot set MTRR at "l4_addr_fmt"-"l4_addr_fmt
			 ", found MTRR from "l4_addr_fmt"-"l4_addr_fmt
			 " type %s\n", addr, addr+size, mtrr_addr,
			 mtrr_addr+mtrr_size, mtrr_mem_type(mtrr_type));
	      return -L4_EINVAL;
	    }
	}
      else
	{
	  /* store number of unused MTRR */
	  if (unused == -1)
	    unused = i;
	}
    }

  if (unused == -1)
    {
      LOG_printf("No MTRR available\n");
      return -L4_EINVAL;
    }

  LOG_printf("Setting MTRR %d to "l4_addr_fmt"-"l4_addr_fmt" type %s\n",
      unused, addr, addr+size, mtrr_mem_type(type));

  /* setup MTRR */
  base = ( (l4_uint64_t)addr & size_and_mask) | type;
  mask = (-(l4_uint64_t)size & size_and_mask) | 0x800;
  wrmsr(0x200 + 2*unused, base);
  wrmsr(0x201 + 2*unused, mask);

  return 0;
}

/**
 * Check if CPU has support for MTRRs
 */
int
mtrr_init(void)
{
  char vendor_id[16];
  unsigned long max;
  unsigned long dummy, ext_features, features;
  int i, found;

  if (!l4util_cpu_has_cpuid())
    /* CPU too old */
    return -L4_EINVAL;

  /* ensure IOPL3 */
  asm volatile ("sti");
  l4util_cpu_cpuid(0, &max, (unsigned long*)vendor_id,
			    (unsigned long*)(vendor_id + 8),
			    (unsigned long*)(vendor_id + 4));

  if (max < 1)
    return -L4_EINVAL;

  l4util_cpu_cpuid(1, &dummy, &dummy, &ext_features, &features);
  if (!(features & 0x1000))
    /* CPU has no support for MTRRs */
    return -L4_EINVAL;

  l4util_cpu_cpuid (0x80000000, &max, &dummy, &dummy, &dummy);
  if (max >= 0x80000008)
    {
      unsigned long virt_phys_addr_size;

      l4util_cpu_cpuid (0x80000008,
			&virt_phys_addr_size, &dummy, &dummy, &dummy);
      size_and_mask  = (1ULL << (virt_phys_addr_size & 0xff)) - 1;
      size_or_mask   = ~size_and_mask;
      size_and_mask &= ~0xfff; /* L4_PAGEMASK is ~0xfffL! */
    }

  /* number of variable range MTRRs */
  num_mtrr = rdmsr(0x00fe) & 0xff;

  LOG_printf("CPU supports %d MTRRs. Allocated:\n", num_mtrr);
  for (i=0, found=0; i<num_mtrr; i++)
    {
      l4_addr_t addr;
      l4_size_t size;
      int       type;
      if (0 == mtrr_get(i, &addr, &size, &type) && size)
	{
	  LOG_printf("  %d: "l4_addr_fmt"-"l4_addr_fmt" (%4dMB) type %s\n",
		     i, addr, addr+size, (unsigned)(size+(1<<20)-1) >> 20,
		     mtrr_mem_type(type));
	  found = 1;
	}
    }
  if (!found)
    LOG_printf("  => no MTRRs active\n");
  available = 1;
  return 0;
}
