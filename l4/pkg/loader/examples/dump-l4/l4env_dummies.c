/* $Id$ */
/**
 * \file   loader/linux/dump-l4/l4env_dummies.c
 * \brief  Dummmy implementations for some unused L4Env functions
 *
 * \date   02/15/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/l4rm/l4rm.h>
#include <l4/env/env.h>

void 
LOG_flush(void)
{
}

int
l4rm_lookup(void * addr, l4dm_dataspace_t * ds, l4_offs_t * offset, 
	    l4_addr_t * map_addr, l4_size_t * map_size)
{
  return -L4_ENOTSUPP;
}

l4_threadid_t
l4env_get_default_dsm(void)
{
  return L4_INVALID_ID;
}
