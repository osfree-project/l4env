/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/phys_addr.c
 * \brief  DMphys, return phys. address of dataspace region
 *
 * \date   11/22/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* DMphys includes */
#include "dm_phys-server.h"
#include "__dataspace.h"
#include "__pages.h"
#include "__debug.h"

/*****************************************************************************
 *** DMphys IDL server functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Return phys. address of dataspace region
 * 
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \param  offset        Offset in dataspace
 * \param  size          Region size, \c L4DM_WHOLE_DS to get the size
 *                       of the contiguous area at offset
 * \retval paddr         Phys. address of offset
 * \retval psize         Size of contiguous region at offset
 * \retval _ev           Flick exception structure, unused
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EINVAL       invalid dataspace id
 *         - \c -L4_EPERM        caller is not a client of the dataspace
 *         - \c -L4_EINVAL_OFFS  offset points beyond end of dataspace
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_mem_phys_addr_component(CORBA_Object _dice_corba_obj,
                                l4_uint32_t ds_id,
                                l4_uint32_t offset,
                                l4_uint32_t size,
                                l4_uint32_t *paddr,
                                l4_uint32_t *psize,
                                CORBA_Environment *_dice_corba_env)
{
  int ret;
  dmphys_dataspace_t * ds;
  page_area_t * area;
  l4_offs_t area_offs;

  LOGdL(DEBUG_PHYS_ADDR,"\n  ds %u, caller %x.%x",
        ds_id,_dice_corba_obj->id.task,_dice_corba_obj->id.lthread);

  /* get dataspace descriptor, caller must be a client */
  ret = dmphys_ds_get_check_client(ds_id,*_dice_corba_obj,&ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	ERROR("DMphys: invalid dataspace id, id %u, caller %x.%x",
	      ds_id,_dice_corba_obj->id.task,_dice_corba_obj->id.lthread);
      else
	ERROR("DMphys: caller %x.%x is not a client of dataspace %d!",
	      _dice_corba_obj->id.task,_dice_corba_obj->id.lthread,ds_id);
#endif
      return ret;
    }

  /* find page area for offset */
  area = dmphys_ds_find_page_area(ds,offset,&area_offs);
  if (area == NULL)
    {
      /* offset points beyond end of dataspace */      
      ERROR("DMphys: invalid offset 0x%08x in dataspace %u (size 0x%08x)!\n",
	    offset,ds_id,dmphys_ds_get_size(ds));
      return -L4_EINVAL_OFFS;
    }

  /* set phys. address / region size */
  *paddr = area->addr + area_offs;
  *psize = area->size - area_offs;
  if ((size != L4DM_WHOLE_DS) && (*psize > size))
    *psize = size;

  LOGdL(DEBUG_PHYS_ADDR,"\n  offset 0x%08x\n" \
        "  area 0x%08x-0x%08x, area offset 0x%08x\n" \
        "  phys. addr 0x%08x, region size 0x%08x",
        offset,area->addr,area->addr + area->size,area_offs,*paddr,*psize);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Check if dataspace is allocated on contiguous phys. memory
 * 
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \retval is_cont       1 if contiguous memory, 0 if not
 * \retval _ev           Flick exception structure, unused
 *	
 * \return 0 on success (check dataspace), error code otherwise:
 *         - \c -L4_EINVAL       invalid dataspace id
 *         - \c -L4_EPERM        caller is not a client of the dataspace
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_mem_is_contiguous_component(CORBA_Object _dice_corba_obj,
                                    l4_uint32_t ds_id,
                                    l4_int32_t *is_cont,
                                    CORBA_Environment *_dice_corba_env)
{
  int ret,num;
  dmphys_dataspace_t * ds;
  
  LOGdL(DEBUG_PHYS_ADDR,"\n  ds %u, caller %x.%x",
        ds_id,_dice_corba_obj->id.task,_dice_corba_obj->id.lthread);
  
  /* get dataspace descriptor, caller must be a client */
  ret = dmphys_ds_get_check_client(ds_id,*_dice_corba_obj,&ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	ERROR("DMphys: invalid dataspace id, id %u, caller %x.%x",
	      ds_id,_dice_corba_obj->id.task,_dice_corba_obj->id.lthread);
      else
	ERROR("DMphys: caller %x.%x is not a client of dataspace %d!",
	      _dice_corba_obj->id.task,_dice_corba_obj->id.lthread,ds_id);
#endif
      return ret;
    }

  /* check number of page areas */
  num = dmphys_ds_get_num_page_areas(ds);
#if DEBUG_PHYS_ADDR
  printf("  %d page areas\n",num);
#endif
  if (num > 1)
    *is_cont = 0;
  else
    *is_cont = 1;

  /* done */
  return 0;
}

