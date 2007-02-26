/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/kinfo.c
 * \brief  L4 kernel info page handling
 *
 * \date   02/05/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/

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
