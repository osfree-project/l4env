/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_fprov/examples/fuxfprov/l4env_dummies.c
 * \brief  Dummmy implementations for some unused L4Env functions
 *
 * \date   02/15/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * This is a copy of loader/linux/fprov-l4/l4env_dummies.c
 */
/*****************************************************************************/
/*
 * (c) 2004 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/l4rm/l4rm.h>
#include <l4/env/env.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/names/libnames.h>

void 
LOG_flush(void)
{
}

int
l4rm_lookup(void * addr, l4_addr_t * map_addr, l4_size_t * map_size,
            l4dm_dataspace_t * ds, l4_offs_t * offset, l4_threadid_t * pager)
{
  return -L4_ENOTSUPP;
}

l4_threadid_t
l4env_get_default_dsm(void)
{
  l4_threadid_t dsm_id;

  if (!names_waitfor_name(L4DM_MEMPHYS_NAME,&dsm_id,60000))
    return L4_INVALID_ID;
  return dsm_id;
}
