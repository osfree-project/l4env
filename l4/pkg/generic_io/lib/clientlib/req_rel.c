/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_io/lib/clientlib/req_rel.c
 * \brief  L4Env I/O Client Library Request/Release Wrapper
 *
 * \date   2007-03-23
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

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
 * \param  start    begin of mem region
 * \param  len      size of mem region
 * \retval offset   offset within memory region
 *
 * \return virtual address of mapped region; 0 on error
 */
/*****************************************************************************/
l4_addr_t l4io_request_mem_region(l4_addr_t start, l4_size_t len, int flags)
{
  int err;
  l4_addr_t vaddr;
  l4_uint32_t area;
  l4_size_t area_len;
  l4_snd_fpage_t region;  /* rcvd fpage */
  CORBA_Environment _env = dice_default_environment;
  unsigned long area_offs;

  /* reserve appropriate area */
  area_len = generic_io_trunc_page(len) ? nLOG2(len) : GENERIC_IO_MIN_PAGEORDER;

  /* Double size of memory area to go sure concerning the offset */
  if ((start & ((1UL << area_len) - 1)) + len > (1UL << area_len))
    area_len++;

  err = l4rm_area_reserve(1UL << area_len,
                          L4RM_LOG2_ALIGNED | L4RM_LOG2_ALLOC, &vaddr, &area);
  if (err)
    {
      LOG_Error("area reservation failed (%d)", err);
      return 0;
    }

  area_offs = start & ((1UL << area_len) - 1);

  /* request mem region */
  _env.rcv_fpage = l4_fpage(vaddr, area_len, L4_FPAGE_RW, 0);
  err = l4_io_request_mem_region_call(&io_l4id, start, len, flags, &region, &_env);
  if (DICE_ERR(err, &_env))
    {
      l4rm_area_release(area);
      return 0;
    }

  vaddr += area_offs;

  /* mapping point of mem region */
  return vaddr;
}

/******************************************************************************/
/**
 * \brief  Search I/O memory region for an address.
 *
 * \param  addr    Address to search for
 * \retval start   Start of memory region if found
 * \retval len     Length of memory region if found.
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int l4io_search_mem_region(l4_addr_t addr,
                           l4_addr_t *start, l4_size_t *len)
{
  int err;

  CORBA_Environment _env = dice_default_environment;

  /* request port region */
  err = l4_io_search_mem_region_call(&io_l4id, addr, start, len, &_env);

  /* done */
  return DICE_ERR(err, &_env);

}

/*****************************************************************************/
/**
 * \brief  Request I/O port region.
 *
 * \param  start  begin of port region
 * \param  len    size of port region
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int l4io_request_region(l4_uint16_t start, l4_uint16_t length)
{
  int err;

  CORBA_Environment _env = dice_default_environment;

  static l4_snd_fpage_t fpages[l4_io_max_fpages];
  l4_size_t num;

  /* open whole I/O space to not fiddle around with alignment constraints */
  _env.rcv_fpage = l4_iofpage(0, L4_WHOLE_IOADDRESS_SPACE, 0);
  err = l4_io_request_region_call(&io_l4id, start, length, &num, fpages, &_env);

  /* done */
  return DICE_ERR(err, &_env);
}

/*****************************************************************************/
/**
 * \brief  Release I/O memory region.
 *
 * \param  start  begin of port region
 * \param  len    size of port region
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

  CORBA_Environment _env = dice_default_environment;

  /* release memory region */
  err = l4_io_release_mem_region_call(&io_l4id, start, len, &_env);

  /* done */
  return DICE_ERR(err, &_env);
}

/*****************************************************************************/
/**
 * \brief  Release I/O port region.
 *
 * \param  start  begin of port region
 * \param  len    size of port region
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int l4io_release_region(l4_uint16_t start, l4_uint16_t len)
{
  int err;

  CORBA_Environment _env = dice_default_environment;

  /* release port region */
  err = l4_io_release_region_call(&io_l4id, start, len, &_env);

  /* done */
  return DICE_ERR(err, &_env);
}

/*****************************************************************************/
/**
 * \brief  Request ISA DMA channel.
 *
 * \param  channel  ISA DMA channel number
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
 * \param  channel  ISA DMA channel number
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
