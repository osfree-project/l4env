/* $Id$ */
/*****************************************************************************/
/**
 * \file   loader/linux/dump-l4/l4env_dummies.c
 * \brief  Dummmy implementations for some unused L4Env functions
 *
 * \date   02/15/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
/*
 * $Log$
 * Revision 1.2  2002/11/25 01:07:47  reuther
 * - adapted to new L4 integer types
 * - adapted to l4sys/l4util/l4env/dm_generic include changes
 *
 * Revision 1.1  2002/05/02 09:41:12  adam
 * - screenshot program for DROPS console running under L4Linux
 * - type "dropsshot --help" for help, if that doesn't suffice beg me
 *   to write a manpage...
 *
 * Revision 1.1  2002/02/18 03:48:12  reuther
 * Emulate some l4env functions.
 *
 */

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
