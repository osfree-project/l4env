/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/lookup.c
 * \brief  Address lookup.
 *
 * \date   02/21/2001
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* private includes */
#include <l4/l4rm/l4rm.h>
#include "__region.h"
#include "__debug.h"

/*****************************************************************************
 *** L4RM client API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Lookup address.
 * 
 * \param   addr         Address
 * \retval  map_start    Map area start address
 * \retval  map_size     Map area size
 * \retval  ds           Dataspace
 * \retval  offset       Offset of ptr in dataspace
 * \retval  pager        External pager
 *	
 * \return  Region type on success (> 0):
 *         - #L4RM_REGION_DATASPACE     region with attached dataspace, \a ds
 *                                      and \a offset contain the dataspace id
 *                                      and the map offset in the dataspace
 *         - #L4RM_REGION_PAGER         region with external pager, \a pager 
 *                                      contains the id of the external pager
 *         - #L4RM_REGION_EXCEPTION     region with exception forward
 *         - #L4RM_REGION_BLOCKED       blocked (unavailable) region
 *         - #L4RM_REGION_UNKNOWN       unknown region
 *         Error codes (< 0):
 *           - -#L4_ENOTFOUND  dataspace not found for address
 *         
 * Return the region type of the region at address \a addr.
 */
/*****************************************************************************/ 
int
l4rm_lookup(const void * addr, l4_addr_t * map_addr, l4_size_t * map_size,
            l4dm_dataspace_t * ds, l4_offs_t * offset, l4_threadid_t * pager)
{
  int type;
  l4rm_region_desc_t * r;
  l4_addr_t a = (l4_addr_t)addr;

  /* lock region list */
  l4rm_lock_region_list();

  LOGdL(DEBUG_LOOKUP, "lookup addr 0x"l4_addr_fmt, a);

  /* lookup address */
  r = l4rm_find_used_region(a);
  if (r == NULL)
    {
      /* not found */
      l4rm_unlock_region_list();
      return -L4_ENOTFOUND;
    }
  
  LOGdL(DEBUG_LOOKUP, "found region 0x"l4_addr_fmt"-0x"l4_addr_fmt
        ", type 0x%08x", r->start, r->end, REGION_TYPE(r));

  *map_addr = r->start;
  *map_size = r->end - r->start + 1;	
  *ds = L4DM_INVALID_DATASPACE;
  *offset = 0;
  *pager = L4_INVALID_ID;
  switch (REGION_TYPE(r))
    {
    case REGION_DATASPACE: 
      LOGd(DEBUG_LOOKUP, "ds %u at "l4util_idfmt", offs 0x"l4_addr_fmt, 
           r->data.ds.ds.id, l4util_idstr(r->data.ds.ds.manager),
	   r->data.ds.offs);

      type = L4RM_REGION_DATASPACE;
      *ds = r->data.ds.ds;
      *offset = (a - r->start) + r->data.ds.offs;
      break;

    case REGION_PAGER:
      LOGd(DEBUG_LOOKUP, "external pager "l4util_idfmt, 
           l4util_idstr(r->data.pager.pager));
      
      type = L4RM_REGION_PAGER;
      *pager = r->data.pager.pager;
      break;
      
    case REGION_EXCEPTION:
      LOGd(DEBUG_LOOKUP, "forward exception");
      
      type = L4RM_REGION_EXCEPTION;
      break;

    case REGION_BLOCKED:
      LOGd(DEBUG_LOOKUP, "blocked region");

      type = L4RM_REGION_BLOCKED;
      break;

    default:
      LOGd(DEBUG_LOOKUP, "unknown region");
      
      type = L4RM_REGION_UNKNOWN;
    }

  /* unlock region list */
  l4rm_unlock_region_list();

  /* done */
  return type;
}

/*****************************************************************************/
/**
 * \brief  Lookup address, return size and type of region
 * 
 * \param  addr          Address
 * \retval map_addr      Start address of region
 * \retval map_size      Size of whole region, beginning at \a map_addr
 * \retval ds            Dataspace
 * \retval offset        Offset of ptr in dataspace
 * \retval pager         External pager
 *	
 * \return Region type on success (>0):
 *         - #L4RM_REGION_FREE          free region
 *         - #L4RM_REGION_RESERVED      reserved region
 *         - #L4RM_REGION_DATASPACE     region with attached dataspace
 *         - #L4RM_REGION_PAGER         region with external pager
 *         - #L4RM_REGION_EXCEPTION     region with exception forward
 *         - #L4RM_REGION_BLOCKED       blocked (unavailable) region
 *         - #L4RM_REGION_UNKNOWN       unknown region
 *         Error codes (< 0):
 *         - -#L4_ENOTFOUND  no region found at given address
 */
/*****************************************************************************/ 
int
l4rm_lookup_region(const void * addr, l4_addr_t * map_addr, 
                   l4_size_t * map_size, l4dm_dataspace_t * ds, 
                   l4_offs_t * offset, l4_threadid_t * pager)
{
  int type;
  l4rm_region_desc_t * r;
  l4_addr_t a = (l4_addr_t)addr;

  /* lock region list */
  l4rm_lock_region_list();

  LOGdL(DEBUG_LOOKUP, "lookup region for addr 0x"l4_addr_fmt, a);

  r = l4rm_find_region(a);
  if (r == NULL)
    {
      /* This should never happen! */
      LOG_Error("region at address 0x"l4_addr_fmt" not found!", a);
      l4rm_unlock_region_list();
      return -L4_ENOTFOUND;
    }

  /* get region type */
  *map_addr = r->start;
  *map_size = r->end - r->start + 1;
  *ds = L4DM_INVALID_DATASPACE;
  *offset = 0;
  *pager = L4_INVALID_ID;
  switch (REGION_TYPE(r))
    {
    case REGION_FREE:
      type = (REGION_AREA(r) == L4RM_DEFAULT_REGION_AREA) ? 
        L4RM_REGION_FREE : L4RM_REGION_RESERVED;
      break;

    case REGION_DATASPACE:
      type = L4RM_REGION_DATASPACE; 
      *ds = r->data.ds.ds;
      *offset = (a - r->start) + r->data.ds.offs;
      break;

    case REGION_PAGER:     
      type = L4RM_REGION_PAGER; 
      *pager = r->data.pager.pager;
      break;      

    case REGION_EXCEPTION: 
      type = L4RM_REGION_EXCEPTION; 
      break;

    case REGION_BLOCKED:   
      type = L4RM_REGION_BLOCKED; 
      break;
      
    default:
      LOG_Error("unknown region state, region flags 0x%08x", r->flags);      
      type = L4RM_REGION_UNKNOWN;
    }
  
  /* done */
  l4rm_unlock_region_list();

  return type;
}
  
