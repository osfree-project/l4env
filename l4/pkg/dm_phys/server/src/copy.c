/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/copy.c
 * \brief  DMphys, create copy of a dataspace
 *
 * \date   11/22/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* Standard includes */
#include <stdio.h>     /* sprintf */
#include <string.h>    /* memcpy / memset */

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* DMphys includes */
#include "dm_phys-server.h"
#include <l4/dm_phys/consts.h>
#include "__dataspace.h"
#include "__internal_alloc.h"
#include "__pages.h"
#include "__dm_phys.h"
#include "__debug.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Create dataspace copy
 * 
 * \param  src           Source dataspace id
 * \param  owner         Destination dataspace owner
 * \param  pool          Memory pool to use
 * \param  dst_addr      Destination dataspace start adddress
 * \param  dst_size      Destination dataspace size
 * \param  dst_align     Destination dataspace alignment
 * \param  src_offs      Offset in source dataspace
 * \param  dst_offs      Offset in destination dataspace
 * \param  num           Number of byte to copy
 * \param  flags         Flags
 * \param  name          Destination dataspace name
 * \retval copy          Dataspace id of copy
 *	
 * \return 0 on success, error code otherwise.
 */
/*****************************************************************************/ 
static int
__create_copy(dmphys_dataspace_t * src, l4_threadid_t owner, 
	      page_pool_t * pool, l4_addr_t dst_addr, l4_size_t dst_size,
	      l4_addr_t dst_align, l4_offs_t src_offs, l4_offs_t dst_offs, 
	      l4_size_t num, l4_uint32_t flags, const char * name, 
	      l4dm_dataspace_t * copy)
{
  int ret;
  dmphys_dataspace_t * dst;
  page_area_t * s_area, * d_area;
  l4_offs_t d_area_offs,s_area_offs;
  l4_offs_t d_offs,s_offs;
  l4_size_t fill,n,sn,dn;
  l4_addr_t d_addr,s_addr;
  char copy_name[20];

  if (name[0] == 0)
    {
      sprintf(copy_name, "copy of ds %d", src->desc->id);
      name = copy_name;
    }

  /* create destination dataspace */
  ret = dmphys_open(owner, pool, dst_addr, dst_size, dst_align, 
                    flags, name, copy);
  if (ret < 0)
    {
      LOGdL(DEBUG_ERRORS, 
            "DMphys: create destination dataspace failed: %d!", ret);
      return ret;
    }
  dst = dmphys_ds_get(copy->id);

  LOGdL(DEBUG_COPY,"copy ds: %u at "l4util_idfmt, 
        copy->id, l4util_idstr(copy->manager));
  
  d_offs = 0;
  if (dst_offs > 0)
    {
      /* set unused start region in destination dataspace to 0 */
      fill = dst_offs;
      while (fill > 0)
	{
	  d_area = dmphys_ds_find_page_area(dst, d_offs, &d_area_offs);
	  ASSERT(d_area != NULL);

	  d_addr = AREA_MAP_ADDR(d_area) + d_area_offs;

	  n = d_area->size - d_area_offs;
	  if (n > fill)
	    n = fill;

#if DEBUG_COPY
	  LOG_printf(" fill 0x%08lx-0x%08lx, 0x%x bytes\n", 
                 d_addr, d_addr + n - 1, n);
#endif
	  memset((void *)d_addr, 0, n);

	  d_offs += n;
	  fill -= n;
	} 		 
    }

  s_offs = src_offs;
  while (num > 0)
    {
      /* copy dataspace */
      d_area = dmphys_ds_find_page_area(dst, d_offs, &d_area_offs);
      s_area = dmphys_ds_find_page_area(src, s_offs, &s_area_offs);
      ASSERT((d_area != NULL) && (s_area != NULL));

      d_addr = AREA_MAP_ADDR(d_area) + d_area_offs;
      s_addr = AREA_MAP_ADDR(s_area) + s_area_offs;

      dn = d_area->size - d_area_offs;
      sn = s_area->size - s_area_offs;
      n = (dn < sn) ? dn : sn;

      if (n > num)
	n = num;

#if DEBUG_COPY
      LOG_printf(" copy 0x%08lx-0x%08lx -> 0x%08lx-0x%08lx (0x%x)\n",
             s_addr, s_addr + n - 1, d_addr, d_addr + n - 1, n);
#endif

      memcpy((void *)d_addr, (void *)s_addr, n);

      d_offs += n;
      s_offs += n;
      num -= n;      
    }
  
  if (d_offs < dst_size)
    {
      /* set unused end region in destination dataspace to 0 */
      fill = dst_size - d_offs;
      while (fill > 0)
	{
	  d_area = dmphys_ds_find_page_area(dst, d_offs, &d_area_offs);
	  ASSERT(d_area != NULL);

	  d_addr = AREA_MAP_ADDR(d_area) + d_area_offs;

	  n = d_area->size - d_area_offs;
	  if (n > fill)
	    n = fill;

#if DEBUG_COPY
	  LOG_printf(" fill 0x%08lx-0x%08lx, 0x%x bytes\n", 
                 d_addr, d_addr + n - 1, n);
#endif
	  memset((void *)d_addr, 0, n);

	  d_offs += n;
	  fill -= n;
	}
    }

  /* we might have allocated internal memory, update memory pool */
  dmphys_internal_alloc_update();

  /* done */
  return 0;
}

/*****************************************************************************
 *** DMphys IDL server functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Create a copy of the dataspace (DMgeneric version)
 * 
 * \param  _dice_corba_obj    Request source
 * \param  ds_id              Source dataspace id
 * \param  src_offs           Offset in source dataspace
 * \param  dst_offs           Offset in destination dataspace
 * \param  num                Number of bytes to copy
 * \param  flags              Flags:
 *                            - #L4DM_CONTIGUOUS        allocate contiguous 
 *                                                      memory area
 *                            - #L4DM_MEMPHYS_SAME_POOL use same pool than 
 *                                                      source dataspace to 
 *                                                      allocate copy
 * \param  name               Copy name
 * \param  _dice_corba_env    Server environment
 * \retval copy               Dataspace id of copy
 *	
 * \return 0 on success (created dataspace copy), error code otherwise:
 *         - -#L4_EINVAL     Invalid source dataspace id
 *         - -#L4_EPERM      Permission denied
 *         - -#L4_ENOHANDLE  Could not create dataspace descriptor
 *         - -#L4_ENOMEM     Out of memory creating copy
 */
/*****************************************************************************/ 
long
if_l4dm_generic_copy_component (CORBA_Object _dice_corba_obj,
                                unsigned long ds_id,
                                unsigned long src_offs,
                                unsigned long dst_offs,
                                unsigned long num,
                                unsigned long flags,
                                const char* name,
                                l4dm_dataspace_t *copy,
                                CORBA_Server_Environment *_dice_corba_env)
{
  int ret;
  dmphys_dataspace_t * ds;
  l4_size_t src_size,dst_size;
  l4_size_t num_copy;
  page_pool_t * pool;

  LOGdL(DEBUG_COPY, "ds %lu, caller "l4util_idfmt,
        ds_id, l4util_idstr(*_dice_corba_obj));

  /* get source dataspace descriptor, caller must be a client */
  ret = dmphys_ds_get_check_client(ds_id, *_dice_corba_obj, &ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	LOGL("DMphys: invalid dataspace id, id %lu, caller "l4util_idfmt,
	     ds_id, l4util_idstr(*_dice_corba_obj));
      else
	LOGL("DMphys: caller "l4util_idfmt" is not a client of dataspace %ld!",
	     l4util_idstr(*_dice_corba_obj), ds_id);
#endif
      return ret;
    }

  /* get memory pool */
  if (flags & L4DM_MEMPHYS_SAME_POOL)
    pool = dmphys_ds_get_pool(ds);
  else
    pool = dmphys_get_default_pool();

  /* get source dataspace size */
  src_size = dmphys_ds_get_size(ds);
  if (src_offs >= src_size)
    {
      /* offset points beyound the end of the dataspace */
      LOGdL(DEBUG_ERRORS,
            "DMphys: invalid source offset 0x%08lx, dataspace size 0x%08zx\n",
	    src_offs, src_size);
      return -L4_EINVAL_OFFS;
    }

  /* calculate amount to copy */
  num_copy = src_size - src_offs;
  if ((num != L4DM_WHOLE_DS) && (num_copy > num))
    num_copy = num;

  /* calculate copy dataspace size, we always use the size specified by the
   * client (dst_offs and number of bytes to copy), only if num is set to
   * L4DM_WHOLE_DS the dataspace size is determined by the actual amount
   * to copy and the destination offset
   */
  if (num == L4DM_WHOLE_DS)
    dst_size = num_copy + dst_offs;
  else
    dst_size = num + dst_offs;
  dst_size = (dst_size + DMPHYS_PAGESIZE - 1) & DMPHYS_PAGEMASK;

  LOGdL(DEBUG_COPY, "copy %zu bytes\n" \
        "  source size 0x%08zx, offset 0x%08lx\n" \
        "  destination size 0x%08zx, offset 0x%08lx, num 0x%08lx",
        num_copy, src_size, src_offs, dst_size, dst_offs, num);

  /* create copy */
  return __create_copy(ds, *_dice_corba_obj, pool, L4DM_MEMPHYS_ANY_ADDR,
                       dst_size, DMPHYS_PAGESIZE, src_offs, dst_offs,
                       num_copy, flags, name, copy);
}
 
/*****************************************************************************/
/**
 * \brief  Create a copy of the dataspace (extended DMphys version)
 * 
 * \param  _dice_corba_obj    Request source
 * \param  ds_id              Source dataspace id
 * \param  src_offs           Offset in source dataspace
 * \param  dst_offs           Offset in destination dataspace
 * \param  num                Number of bytes to copy
 * \param  dst_pool           Memory pool to use to allocate destination 
 *                            dataspace
 * \param  dst_addr           Phys. address of destination dataspace
 *                            (#L4DM_MEMPHYS_ANY_ADDR ... find suitable area)
 * \param  dst_size           Size of destination dataspace
 * \param  dst_align          Alignment of destination dataspace
 * \param  flags              Flags:
 *                            - #L4DM_CONTIGUOUS        allocate contiguous 
 *                                                      memory area
 *                            - #L4DM_MEMPHYS_SAME_POOL use same pool than 
 *                                                      source dataspace to 
 *                                                      allocate copy
 * \param  name               Copy name
 * \param  _dice_corba_env    Server environment
 * \retval copy               Dataspace id of copy
 *	
 * \return 0 on success (created dataspace copy), error code otherwise:
 *         - -#L4_EINVAL     Invalid argument
 *         - -#L4_EPERM      Permission denied
 *         - -#L4_ENOHANDLE  Could not create dataspace descriptor
 *         - -#L4_ENOMEM     Out of memory creating copy
 */
/*****************************************************************************/ 
long
if_l4dm_memphys_dmphys_copy_component (CORBA_Object _dice_corba_obj,
                                       unsigned long ds_id,
                                       unsigned long src_offs,
                                       unsigned long dst_offs,
                                       unsigned long num,
                                       unsigned long dst_pool,
                                       unsigned long dst_addr,
                                       unsigned long dst_size,
                                       unsigned long dst_align,
                                       unsigned long flags,
                                       const char* name,
                                       l4dm_dataspace_t *copy,
                                       CORBA_Server_Environment *_dice_corba_env)
{
  int ret;
  dmphys_dataspace_t * ds;
  l4_size_t src_size,dst_ds_size;
  l4_size_t num_copy;
  page_pool_t * pool;

  /* get source dataspace descriptor, caller must be a client */
  ret = dmphys_ds_get_check_client(ds_id, *_dice_corba_obj, &ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	LOGL("DMphys: invalid dataspace id, id %lu, caller "l4util_idfmt,
	     ds_id, l4util_idstr(*_dice_corba_obj));
      else
	LOGL("DMphys: caller "l4util_idfmt" is not a client of dataspace %ld!",
	     l4util_idstr(*_dice_corba_obj), ds_id);
#endif
      return ret;
    }

  /* get memory pool */
  if (flags & L4DM_MEMPHYS_SAME_POOL)
    pool = dmphys_ds_get_pool(ds);
  else
    {
      pool = dmphys_get_page_pool(dst_pool);
      if (pool == NULL)
	{
	  LOGdL(DEBUG_ERRORS, "DMphys: invalid page pool %ld!", dst_pool);
	  return -L4_EINVAL;
	}
    }

  /* get source dataspace size */
  src_size = dmphys_ds_get_size(ds);
  if (src_offs >= src_size)
    {
      /* offset points beyound the end of the dataspace */
      LOGdL(DEBUG_ERRORS,
            "DMphys: invalid source offset 0x%08lx, dataspace size 0x%08zx",
	    src_offs, src_size);
      return -L4_EINVAL_OFFS;
    }

  /* calculate amount to copy */
  num_copy = src_size - src_offs;
  if ((num != L4DM_WHOLE_DS) && (num_copy > num))
    num_copy = num;

  /* calculate the copy dataspace size, it is the max. of the size specified
   * in dst_size and the size determined by the offset in the destination
   * dataspace and the actual amount to copy
   */
  dst_ds_size = dst_offs + num_copy;
  if (dst_size > dst_ds_size)
    dst_ds_size = dst_size;

  LOGdL(DEBUG_COPY, "ds %lu, caller "l4util_idfmt", copy %zu bytes\n" \
        " source size 0x%08zx, offset 0x%08lx\n" \
        " destination offset 0x%08lx, num 0x%08lx\n" \
        " destination size 0x%08zx (caller 0x%08lx)\n" \
        " destination at 0x%08lx, align 0x%08lx",
        ds_id, l4util_idstr(*_dice_corba_obj), num_copy,
        src_size, src_offs, dst_offs, num, dst_ds_size, dst_size,
        dst_addr, dst_align);

  /* create copy */
  return __create_copy(ds, *_dice_corba_obj, pool, dst_addr, dst_ds_size,
                       dst_align, src_offs, dst_offs, num_copy, flags,
                       name, copy);
}
