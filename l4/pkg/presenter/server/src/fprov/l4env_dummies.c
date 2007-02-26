/* $Id$ */
/*****************************************************************************/
/**
 * \file   loader/linux/fprov-l4/l4env_dummies.c
 * \brief  Dummmy implementations for some unused L4Env functions
 *
 * \date   02/15/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
/*
 * $Log$
 * Revision 1.3  2004/02/12 00:30:37  reuther
 * - adapted to changes in l4rm
 *
 * Revision 1.2  2003/12/23 08:48:08  js712688
 * removed a function because of conflicts with emul_l4rm.c
 *
 * Revision 1.1  2003/11/28 11:24:54  js712688
 * initial revision.
 *
 * Revision 1.2  2002/11/25 03:27:53  reuther
 * - adapted to new L4 integer types
 * - adapted to l4sys/l4util/l4env/dm_generic include changes
 *
 * Revision 1.1  2002/09/24 11:38:33  fm3
 * - moved here from linux subdir
 *
 * Revision 1.2  2002/03/08 08:21:06  fm3
 * - transfer ownership of resulting ds to calling client
 *
 * Revision 1.1  2002/02/18 03:48:13  reuther
 * Emulate some l4env functions.
 *
 */

#include <l4/sys/types.h>
#include <l4/env/env.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/names/libnames.h>

l4_threadid_t
l4env_get_default_dsm(void)
{
  l4_threadid_t dsm_id;

  if (!names_waitfor_name(L4DM_MEMPHYS_NAME,&dsm_id,60000))
    return L4_INVALID_ID;
  return dsm_id;
}
