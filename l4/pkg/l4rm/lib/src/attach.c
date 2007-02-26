/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/attach.c
 * \brief  Attach new dataspace.
 *
 * \date   06/03/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
//#include <l4/dm_generic/dm_generic.h>
#include <l4/sys/consts.h>
#include <l4/util/bitops.h>
#include <l4/util/macros.h>

/* private includes */
#include <l4/l4rm/l4rm.h>
#include "__region.h"
#include "__region_alloc.h"
//#include "__libl4rm.h"
#include "__config.h"
#include "__debug.h"

// was in __libl4rm.h
extern l4_threadid_t l4rm_service_id;

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Attach dataspace to region.
 * 
 * \param  ds            dataspace id
 * \param  area          area id
 * \param  addr          region start address 
 *                       (ADDR_FIND ... find suitable region)
 * \param  size          region size
 * \param  ds_offs       offset in dataspace
 * \param  flags         flags:
 *                       - \c L4DM_RO           attach read-only
 *                       - \c L4DM_RW           attach read/write
 *                       - \c L4RM_LOG2_ALIGNED find a 2^(log2(size) + 1)
 *                                              aligned region
 *                       - \c L4RM_LOG2_ALLOC   allocate the whole 
 *                                              2^(log2(size) + 1) sized area
 *                       - \c L4RM_MAP          immediately map area
 *                       - \c MODIFY_DIRECT     add new region without locking
 *                                              the region list and calling the
 *                                              region mapper thread
 * \retval addr          start address of region
 *	
 * \return 0 on success (dataspace attached to region), error code otherwise:
 *         - \c -L4_ENOMEM  out of memory allocating region descriptor
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_ENOMAP  no suitable region found
 *         - \c -L4_EUSED   region already used
 *         - \c -L4_EIPC    error calling region mapper
 *         - \c -L4_EPERM   could not set the access rights for the region
 *                          mapper thread at the dataspace manager. 
 */
/*****************************************************************************/ 
static int
__attach(l4dm_dataspace_t * ds, 
	 l4_uint32_t area, 
	 l4_addr_t * addr, 
	 l4_size_t size, 
	 l4_offs_t ds_offs, 
	 l4_uint32_t flags)
{
  l4rm_region_desc_t * r;
  l4_uint32_t align,old_rights;
  l4_uint32_t rights = (flags & L4DM_RIGHTS_MASK);
  l4_size_t real_size;
  int ret;

  /* sanity checks */
  if (l4dm_is_invalid_ds(*ds))
    return -L4_EINVAL;

  LOGdL(DEBUG_ATTACH,"DS %u at %x.%x, offset 0x%x, area 0x%x",
        ds->id,ds->manager.id.task,ds->manager.id.lthread,ds_offs,area);
  
  /* lock region list */
  l4rm_lock_region_list_direct(flags);

  /* get current access rights to the dataspace */
  old_rights = l4rm_get_access_rights(ds);

  /* find suitable region */
  if (*addr == ADDR_FIND)
    {
      ret = l4rm_find_free_region(size,area,flags,addr);
      if (ret < 0)
	{
	  /* nothing found */
	  l4rm_unlock_region_list_direct(flags);
	  return -L4_ENOMAP;
	}
    }

  LOGdL(DEBUG_ATTACH,"region <%08x,%08x>",*addr,*addr + size);
  
  /* allocate new region descriptor */
  r = l4rm_region_desc_alloc();
  if (r == NULL)
    {
      /* out of memory */
      l4rm_unlock_region_list_direct(flags);
      return -L4_ENOMEM;
    }

  /* setup new region */
  real_size = size;
  if ((flags & L4RM_LOG2_ALIGNED) && (flags & L4RM_LOG2_ALLOC))
    {
      /* round size to next log2 size */
      align = l4util_bsr(size);
      if (size > (1UL << align))
	align++;

      size = 1UL << align;
    }
  r->ds = *ds;
  r->offs = ds_offs;
  r->start = *addr;
  r->end = *addr + size - 1;
  r->rights = rights;
  SET_AREA(r,area);
  SET_ATTACHED(r);

  /* insert region */
  ret = l4rm_insert_region(r);
  if (ret < 0)
    {
      /* insert failed, region probably already used  */
      l4rm_region_desc_free(r);
      l4rm_unlock_region_list_direct(flags);
      return ret;
    }

  /* insert region into region tree, this will call the region mapper thread */
  ret = l4rm_tree_insert_region(r,flags);
  if (ret < 0)
    {
      if (ret == -L4_EEXISTS)
	/* this should never happen */
	Panic("corrupted region list or AVL tree!");

      l4rm_free_region(r);
      l4rm_unlock_region_list_direct(flags);
      return ret;
    }
 
  /* adapt access rights of the region mapper thread if necessary */
  if (rights & ~old_rights)
    {
      /* new rights exeed old rights, call dataspace manager */
      LOGdL(DEBUG_ATTACH,"expand rights, old 0x%02x, new 0x%02x",
            old_rights,old_rights | rights);
      
      ret = l4dm_share(ds,l4rm_service_id,rights);
      if (ret < 0)
	{
	  l4rm_tree_remove_region(r->start,flags,&r);
	  l4rm_free_region(r);
	  l4rm_unlock_region_list_direct(flags);
	  return ret;
	}
    }

  /* unlock region list */
  l4rm_unlock_region_list_direct(flags);

  if ((flags & L4RM_MAP) && !(flags & MODIFY_DIRECT))
    {
      /* immediately map attached region */
      LOGdL(DEBUG_ATTACH,"map attached region\n" \
            "  region 0x%08x-0x%08x, size 0x%x, rights 0x%02x",
            *addr, *addr + size - 1, size, rights);

      ret = l4dm_map((void *)*addr,real_size,rights);
      if (ret < 0)
	printf("map attached region failed (%d), ignored!\n",ret);
    }

  /* done */
  return 0;
}

/*****************************************************************************
 *** L4RM client functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Attach dataspace.
 * 
 * \param  ds            Dataspace id
 * \param  size          Size 
 * \param  ds_offs       Offset in dataspace
 * \param  flags         Flags:
 *                       - \c L4DM_RO           attach read-only
 *                       - \c L4DM_RW           attach read/write
 *                       - \c L4RM_LOG2_ALIGNED find a 2^(log2(size) + 1)
 *                                              aligned region
 *                       - \c L4RM_LOG2_ALLOC   allocate the whole 
 *                                              2^(log2(size) + 1) sized area
 *                       - \c L4RM_MAP          immediately map area
 * \retval addr          Start address
 *	
 * \return 0 on success (dataspace attached to region at address \a addr), 
 *         error code otherwise:
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_ENOMEM  out of memory allocating descriptors
 *         - \c -L4_ENOMAP  no region found
 *         - \c -L4_EIPC    error calling region mapper
 * 
 * Find an unused map region and attach dataspace area 
 * (\a ds_offs, \a ds_offs + \a size) to that region.
 */
/*****************************************************************************/ 
int
l4rm_attach(l4dm_dataspace_t * ds, 
	    l4_size_t size, 
	    l4_offs_t ds_offs,
	    l4_uint32_t flags, 
	    void ** addr)
{
  /* align size */
  size = (size + L4_PAGESIZE - 1) & L4_PAGEMASK;
  
  /* attach */
  *addr = (void *)ADDR_FIND;
  return __attach(ds,L4RM_DEFAULT_REGION_AREA,(l4_addr_t *)addr,size,ds_offs,
		  flags);
}

/*****************************************************************************/
/**
 * \brief Attach dataspace to specified region.
 * 
 * \param  ds            Dataspace id
 * \param  addr          Start address
 * \param  size          Size 
 * \param  ds_offs       Offset in dataspace
 * \param  flags         Flags:
 *                       - \c L4DM_RO   attach read-only
 *                       - \c L4DM_RW   attach read/write
 *                       - \c L4RM_MAP  immediately map area
 *	
 * \return 0 on success (dataspace attached to region at \a addr), error code
 *         otherwise:
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_EUSED   region already used
 *         - \c -L4_ENOMEM  out of memory allocating descriptors
 *         - \c -L4_ENOMAP  no region found
 *         - \c -L4_EIPC    error calling region mapper
 *         
 * Attach dataspace to region at \a addr.
 */
/*****************************************************************************/ 
int
l4rm_attach_to_region(l4dm_dataspace_t * ds, 
		      void * addr, 
		      l4_size_t size,
		      l4_offs_t ds_offs, 
		      l4_uint32_t flags)
{
  l4_offs_t offs;
  l4_addr_t a = (l4_addr_t)addr;
  
  /* align address / size */
  offs = a & ~(L4_PAGEMASK);
  if (offs > 0)
    {
      printf("L4RM: fixed alignment, 0x%08x -> 0x%08x!\n",
             a,a & L4_PAGEMASK);
      a &= L4_PAGEMASK;
      size += offs;
    }
  size = (size + L4_PAGESIZE - 1) & L4_PAGEMASK;
  
  /* attach */
  return __attach(ds,L4RM_DEFAULT_REGION_AREA,&a,size,ds_offs,flags);
}

/*****************************************************************************/
/**
 * \brief Attach dataspace to area.
 * 
 * \param  ds            Dataspace id
 * \param  area          Area id
 * \param  size          Size 
 * \param  ds_offs       Offset in dataspace
 * \param  flags         Flags:
 *                       - \c L4DM_RO           attach read-only
 *                       - \c L4DM_RW           attach read/write
 *                       - \c L4RM_LOG2_ALIGNED find a 2^(log2(size) + 1)
 *                                              aligned region
 *                       - \c L4RM_LOG2_ALLOC   allocate the whole 
 *                                              2^(log2(size) + 1) sized area
 *                       - \c L4RM_MAP          immediately map area
 * \retval addr          Start address 
 *	
 * \return 0 on success (dataspace attached to region at \a addr), error code
 *         otherwise:
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_ENOMEM  out of memory allocating descriptors
 *         - \c -L4_ENOMAP  no region found
 *         - \c -L4_EIPC    error calling region mapper
 * 
 * Attach dataspace to area \a area. An area is a region in the address space
 * reserved by l4rm_area_reserve().
 */
/*****************************************************************************/ 
int 
l4rm_area_attach(l4dm_dataspace_t * ds, 
		 l4_uint32_t area, 
		 l4_size_t size,
		 l4_offs_t ds_offs, 
		 l4_uint32_t flags, 
		 void ** addr)
{
  /* align size */
  size = (size + L4_PAGESIZE - 1) & L4_PAGEMASK;
  
  /* attach */
  *addr = (void *)ADDR_FIND;
  return __attach(ds,area,(l4_addr_t *)addr,size,ds_offs,flags);
}
  
/*****************************************************************************/
/**
 * \brief Attach dataspace to specified region in area.
 * 
 * \param  ds            Dataspace id
 * \param  area          Area id
 * \param  addr          Start address
 * \param  size          Size
 * \param  ds_offs       Offset in dataspace
 * \param  flags         Flags
 *                       - \c L4DM_RO   attach read-only
 *                       - \c L4DM_RW   attach read/write
 *                       - \c L4RM_MAP  immediately map area
 *	
 * \return 0 on success (dataspace attached to region at \a addr), error code
 *         otherwise:
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_EUSED   region already used
 *         - \c -L4_ENOMEM  out of memory allocating descriptors
 *         - \c -L4_ENOMAP  no region found
 *         - \c -L4_EIPC    error calling region mapper
 *
 * Attach dataspace to region at \a addr in area \a area.
 */
/*****************************************************************************/ 
int 
l4rm_area_attach_to_region(l4dm_dataspace_t * ds, 
			   l4_uint32_t area, 
			   void * addr, 
			   l4_size_t size, 
			   l4_offs_t ds_offs, 
			   l4_uint32_t flags)
{
  l4_offs_t offs;
  l4_addr_t a = (l4_addr_t)addr;
  
  /* align address */
  offs = a & ~(L4_PAGEMASK);
  if (offs > 0)
    {
      printf("L4RM: fixed alignment, 0x%08x -> 0x%08x!\n",
             a,a & L4_PAGEMASK);
      a &= L4_PAGEMASK;
      size += offs;
    }
  size = (size + L4_PAGESIZE - 1) & L4_PAGEMASK;
  
  /* attach */
  return __attach(ds,area,&a,size,ds_offs,flags);
}
  
/*****************************************************************************
 * The next functions call __attach directly, not through the region mapper
 * thread. Therefore NO synchronization is done!
 * These functions are intended to be used by the startup code of a task to
 * setup the initial region map. User functions should use the
 * l4rm_attach_* functions to attach new dataspaces, they call the
 * region mapper thread to serialize accesses to the region list / AVL tree.
 * All functions have the same in/out/ret paramaters like the functions
 * use the region mapper.
 *****************************************************************************/

/*****************************************************************************
 * l4rm_direct_attach
 *****************************************************************************/
int
l4rm_direct_attach(l4dm_dataspace_t * ds, 
		   l4_size_t size, 
		   l4_offs_t ds_offs, 
		   l4_uint32_t flags, 
		   void ** addr)
{
  /* align size */
  size = (size + L4_PAGESIZE - 1) & L4_PAGEMASK;

  /* attach */
  *addr = (void *)ADDR_FIND;
  return __attach(ds,L4RM_DEFAULT_REGION_AREA,(l4_addr_t *)addr,size,ds_offs,
		  flags | MODIFY_DIRECT);
}

/*****************************************************************************
 * l4rm_direct_attach_to_region
 *****************************************************************************/
int 
l4rm_direct_attach_to_region(l4dm_dataspace_t * ds, 
			     void * addr, 
			     l4_size_t size, 
			     l4_offs_t ds_offs, 
			     l4_uint32_t flags)
{
  l4_offs_t offs;
  l4_addr_t a = (l4_addr_t)addr;

  /* allign address */
  offs = a & ~(L4_PAGEMASK);
  if (offs > 0)
    {
      printf("L4RM: fixed alignment, 0x%08x -> 0x%08x!\n",
             a,a & L4_PAGEMASK);
      a &= L4_PAGEMASK;
      size += offs;
    }
  size = (size + L4_PAGESIZE - 1) & L4_PAGEMASK;
  
  /* attach */
  return __attach(ds,L4RM_DEFAULT_REGION_AREA,&a,size,ds_offs,
		  flags | MODIFY_DIRECT);
}

/*****************************************************************************
 * l4rm_direct_area_attach
 *****************************************************************************/
int
l4rm_direct_area_attach(l4dm_dataspace_t * ds, 
			l4_uint32_t area, 
			l4_size_t size,
			l4_offs_t ds_offs, 
			l4_uint32_t flags, 
			void ** addr)
{
  /* align size */
  size = (size + L4_PAGESIZE - 1) & L4_PAGEMASK;

  /* attach */
  *addr = (void *)ADDR_FIND;
  return __attach(ds,area,(l4_addr_t *)addr,size,ds_offs,
		  flags | MODIFY_DIRECT);
}
  
/*****************************************************************************
 * l4rm_direct_area_attach_to_region
 *****************************************************************************/
int 
l4rm_direct_area_attach_to_region(l4dm_dataspace_t * ds, 
				  l4_uint32_t area,
				  void * addr, 
				  l4_size_t size, 
				  l4_offs_t ds_offs, 
				  l4_uint32_t flags)
{
  l4_offs_t offs;
  l4_addr_t a = (l4_addr_t)addr;

  /* allign address */
  offs = a & ~(L4_PAGEMASK);
  if (offs > 0)
    {
      printf("L4RM: fixed alignment, 0x%08x -> 0x%08x!\n",
             a,a & L4_PAGEMASK);
      a &= L4_PAGEMASK;
      size += offs;
    }
  size = (size + L4_PAGESIZE - 1) & L4_PAGEMASK;
  
  /* attach */
  return __attach(ds,area,&a,size,ds_offs,flags | MODIFY_DIRECT);
}
  
