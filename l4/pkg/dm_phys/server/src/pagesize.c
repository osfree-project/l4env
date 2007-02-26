/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/pagesize.c
 * \brief  DMphys, check pagesize for a dataspace region
 *
 * \date   02/09/2002
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
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* DMphys/private include */
#include "dm_phys-server.h"
#include "__dataspace.h"
#include "__pages.h"
#include "__memmap.h"
#include "__debug.h"

/*****************************************************************************
 *** DMphys IDL server functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Check pagesize for dataspace region
 * 
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \param  offs          Offset in dataspace
 * \param  size          Dataspce region size
 * \param  pagesize      Pagesize
 * \retval ok            1 if dataspace region can be mapped with given
 *                       pagesize, 0 if not
 * \retval _ev           Flick exception structure, unused
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EPERM       operation not permitted
 *         - \c -L4_EINVAL      invalid dataspace id
 *         - \c -L4_EINVAL_OFFS offset points beyond end of dataspace
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_memphys_server_dmphys_pagesize(sm_request_t * request, 
				       l4_uint32_t ds_id, 
				       l4_uint32_t offs, 
				       l4_uint32_t size, 
				       l4_uint32_t pagesize, 
				       l4_uint32_t * ok, 
				       sm_exc_t * _ev)
{
  int ret;
  dmphys_dataspace_t * ds;
  l4_threadid_t caller = request->client_tid;
  page_area_t * area;
  l4_offs_t area_offset;
  
  *ok = 0;

  /* get dataspace descriptor, caller must be a client */
  ret = dmphys_ds_get_check_client(ds_id,caller,&ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	ERROR("DMphys: invalid dataspace id, id %u, caller %x.%x",
	      ds_id,caller.id.task,caller.id.lthread);
      else
	ERROR("DMphys: caller %x.%x is not a client of dataspace %d!",
	      caller.id.task,caller.id.lthread,ds_id);
#endif
      return ret;
    }

  if ((pagesize != DMPHYS_LOG2_PAGESIZE) && 
      (pagesize != DMPHYS_LOG2_4MPAGESIZE))
    {
      /* unsupported pagesize, we just print a warning, but continue to
       * check, the area migth be available with a larger pagesize */
      Msg("DMphys: warning, unsupported pagesize %d!\n",pagesize);
    }

  /* find offset in dataspace */
  area = dmphys_ds_find_page_area(ds,offs,&area_offset);
  if (area == NULL)
    return -L4_EINVAL_OFFS;

  /* check size, only contiguous areas can be mapped with a pagesize 
   * larger than DMPHYS_PAGESIZE */
  if ((area_offset + size) > area->size)
    {
      /* test failed */
#if DEBUG_PAGESIZE
      INFO("\n");
      DMSG("  area not contiguous\n");
#endif

      return 0;
    }

  /* have contiguous areas, check pagesize */
  if (dmphys_memmap_check_pagesize(area->addr + area_offset,size,pagesize))
    /* test succeeded */
    *ok = 1;

  /* done */
  return 0;
}

