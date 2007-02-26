/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/kinfo.c
 * \brief  L4 kernel info page handling
 *
 * \date   02/05/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/sys/kernel.h>

/* DMphys includes */
#include "__kinfo.h"
#include "__sigma0.h"

static l4_kernel_info_t * kinfo;

/*****************************************************************************
 *** DMphys internal API function
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Return max. phys memory address
 *	
 * \return Max. memory address
 */
/*****************************************************************************/ 
#ifdef ARCH_x86
l4_addr_t
dmphys_kinfo_mem_high(void)
{
  if (!kinfo)
    kinfo = dmphys_sigma0_kinfo();

  return kinfo->main_memory.high;
}

l4_addr_t
dmphys_kinfo_mem_low(void)
{
  return 0;
}

void
dmphys_kinfo_init_ram_base(void)
{
  /* Keep at zero */
}

#else

enum lowhigh {
  low, high
};

static
l4_addr_t
dmphys_kinfo_get_conventional_mem(enum lowhigh t)
{
  int i, n;
  l4_addr_t val = t == low ? ~0UL : 0UL;

  if (!kinfo)
    kinfo = dmphys_sigma0_kinfo();

  l4_kernel_info_mem_desc_t *md = l4_kernel_info_get_mem_descs(kinfo);

  n = l4_kernel_info_get_num_mem_descs(kinfo);

  /* Find conventional memory in memory descriptors */
  for (i = 0; i < n; md++, i++)
    if (   l4_kernel_info_get_mem_desc_type(md) == l4_mem_type_conventional
	&& !l4_kernel_info_get_mem_desc_is_virtual(md))
      {
	switch (t)
	  {
	  case low:
	    if (l4_kernel_info_get_mem_desc_start(md) < val)
	      val = l4_kernel_info_get_mem_desc_start(md);
	    break;
	  case high:
	    if (l4_kernel_info_get_mem_desc_end(md) > val)
	      val = l4_kernel_info_get_mem_desc_end(md);
	    break;
	  }
      }

  return val;
}

l4_addr_t
dmphys_kinfo_mem_high(void)
{
  return dmphys_kinfo_get_conventional_mem(high) + 1;
}

l4_addr_t
dmphys_kinfo_mem_low(void)
{
  ram_base = dmphys_kinfo_get_conventional_mem(low);
  return ram_base;
}

void
dmphys_kinfo_init_ram_base(void)
{
  dmphys_kinfo_mem_low();
}

#endif
