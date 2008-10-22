/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/kinfo.cc
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
#include <l4/sys/kip.h>

/* DMphys includes */
#include "__kinfo.h"
#include "__sigma0.h"

using L4::Kip::Mem_desc;

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

enum lowhigh {
  low, high
};

static
l4_addr_t
dmphys_kinfo_get_conventional_mem(enum lowhigh t)
{
  l4_addr_t val = t == low ? ~0UL : 0UL;
  l4_addr_t end_trunc;

  if (!kinfo)
    kinfo = dmphys_sigma0_kinfo();

  Mem_desc *md = Mem_desc::first(kinfo);
  Mem_desc *end = md + Mem_desc::count(kinfo);

  /* Find conventional memory in memory descriptors */
  for (; md != end; ++md)
    if (md->type() == Mem_desc::Conventional && !md->is_virtual())
      {
	switch (t)
	  {
	  case low:
	    if (l4_round_page(md->start()) < val)
	      val = l4_round_page(md->start());
	    break;
	  case high:
            /* Limit the upper bound of physical memory to the last address
             * of the last fully available page, otherwise __map() does not
             * work correctly. Even works if md->end is 0xffffffff on 32-bit
             * systems. */
            end_trunc = l4_trunc_page(md->end() + 1) - 1;
	    if (end_trunc > val)
	      val = end_trunc;
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
  if (ram_base < L4_SUPERPAGESIZE)
    ram_base = 0;
  return ram_base;
}

void
dmphys_kinfo_init_ram_base(void)
{
  dmphys_kinfo_mem_low();
}

l4_kernel_info_t *dmphys_kinfo()
{
  if (!kinfo)
    kinfo = dmphys_sigma0_kinfo();
  return kinfo;
}
