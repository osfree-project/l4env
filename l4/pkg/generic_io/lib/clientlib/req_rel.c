/* $Id$ */
/*****************************************************************************/
/**
 * \file	generic_io/lib/src/req_rel.c
 *
 * \brief	L4Env I/O Client Library Request/Release Wrapper
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/l4rm/l4rm.h>

/* io includes */
#include <l4/generic_io/libio.h>

/* local */
#include "internal.h"
#include "__macros.h"

/*****************************************************************************/
/**
 * \brief  Request I/O memory region.
 *
 * \param  start	begin of mem region
 * \param  len		size of mem region
 * \retval offset	offset within memory region
 *
 * \return virtual address of mapped region; 0 on error
 */
/*****************************************************************************/
l4_addr_t l4io_request_mem_region(l4_addr_t start, l4_size_t len,
    				  l4_umword_t *offset)
{
  int err;

  l4_addr_t vaddr;
  l4_uint32_t area;
  l4_size_t area_len;

  l4_fpage_t rfp;		/* rcv fpage desc */
  l4_snd_fpage_t region;	/* rcvd fpage */
  sm_exc_t _exc;
  unsigned int poss_phys;

  /* reserve appropriate area */
  if (len & L4_SUPERPAGEMASK)
    {
      area_len = nLOG2(len);
    }
  else
    area_len = L4_LOG2_SUPERPAGESIZE;

  /* Double size of memory area to go sure concerning the offset */
  area_len++;

  err = l4rm_area_reserve(1 << area_len,
			  L4RM_LOG2_ALIGNED | L4RM_LOG2_ALLOC, &vaddr, &area);
  if (err)
    {
      ERROR("area reservation failed (%d)", err);
      return 0;
    }

  /* request mem region */
  rfp = l4_fpage(vaddr, area_len, L4_FPAGE_RW, 0);

  err = l4_io_request_mem_region(io_l4id, rfp, start, len, &region, offset, &_exc);
  if (FLICK_ERR(err, &_exc))
    return 0;

  poss_phys = start - *offset;
  vaddr += poss_phys - ((poss_phys >> L4_LOG2_SUPERPAGESIZE) << L4_LOG2_SUPERPAGESIZE);

  /* mapping point of mem region */
  return vaddr;
}

/*****************************************************************************/
/**
 * \brief  Request I/O port region.
 *
 * \param  start	begin of port region
 * \param  len		size of port region
 *
 * \return 0 on success; negative error code otherwise
 *
 * \todo I/O flexpages
 */
/*****************************************************************************/
int l4io_request_region(l4_addr_t start, l4_size_t len)
{
  int err;

  sm_exc_t _exc;

  /* request port region */
  err = l4_io_request_region(io_l4id, start, len, &_exc);

  /* done */
  return FLICK_ERR(err, &_exc);
}

/*****************************************************************************/
/**
 * \brief  Release I/O memory region.
 *
 * \param  start	begin of port region
 * \param  len		size of port region
 *
 * \return 0 on success; negative error code otherwise
 *
 * \todo undo area reservation at l4rm but we need the area id for [start,
 * start+len] here
 */
/*****************************************************************************/
int l4io_release_mem_region(l4_addr_t start, l4_size_t len)
{
  int err;

  sm_exc_t _exc;

  /* request port region */
  err = l4_io_release_mem_region(io_l4id, start, len, &_exc);

  /* done */
  return FLICK_ERR(err, &_exc);
}

/*****************************************************************************/
/**
 * \brief  Release I/O port region.
 *
 * \param  start	begin of port region
 * \param  len		size of port region
 *
 * \return 0 on success; negative error code otherwise
 *
 * \todo I/O flexpages
 */
/*****************************************************************************/
int l4io_release_region(l4_addr_t start, l4_size_t len)
{
  int err;

  sm_exc_t _exc;

  /* request port region */
  err = l4_io_release_region(io_l4id, start, len, &_exc);

  /* done */
  return FLICK_ERR(err, &_exc);
}

/*****************************************************************************/
/**
 * \brief  Request ISA DMA channel.
 *
 * \param  channel	ISA DMA channel number
 *
 * \return 0 on success; negative error code otherwise
 *
 * \krishna Not yet implemented.
 */
/*****************************************************************************/
int l4io_request_dma(unsigned int channel)
{
  return -L4_ESKIPPED;
}

/*****************************************************************************/
/**
 * \brief  Release ISA DMA channel.
 *
 * \param  channel	ISA DMA channel number
 *
 * \return 0 on success; negative error code otherwise
 *
 * \krishna Not yet implemented.
 */
/*****************************************************************************/
int l4io_release_dma(unsigned int channel)
{
  return -L4_ESKIPPED;
}
