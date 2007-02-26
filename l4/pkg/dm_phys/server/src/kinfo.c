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
l4_addr_t
dmphys_kinfo_mem_high(void)
{
  l4_kernel_info_t * kinfo = dmphys_sigma0_kinfo();

  return kinfo->main_memory.high;
}
