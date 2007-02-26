/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/include/__dataspace.h
 * \brief  Internal dataspace descriptor handling.
 *
 * \date   01/28/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _DM_PHYS___DATASPACE_H
#define _DM_PHYS___DATASPACE_H

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/cdefs.h>
#include <l4/util/macros.h>
#include <l4/dm_generic/dsmlib.h>

/* DMphys includes */
#include "__pages.h"

/*****************************************************************************
 *** typedefs
 *****************************************************************************/

/**
 * dataspace descriptor
 */
typedef struct dmphys_dataspace
{
  dsmlib_ds_desc_t *        desc;   ///< global dataspace descriptor

  /* memory pages */
  page_area_t *             pages;  ///< page list
  page_pool_t *             pool;   ///< page pool used to allocate the pages

  l4_size_t                 size;   ///< dataspace size
  l4_uint32_t               flags;  ///< dataspace flags

} dmphys_dataspace_t;

#define DS_IS_CONTIGUOUS(ds)  ((ds)->flags & L4DM_CONTIGUOUS)

/**
 * Dataspace list iterator function type 
 */
typedef void (* dmphys_ds_iter_fn_t) (dmphys_dataspace_t * ds, void * data);

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

int
dmphys_ds_init(void);

dmphys_dataspace_t *
dmphys_ds_create(l4_threadid_t owner, 
		 const char * name, 
		 l4_uint32_t flags);

void
dmphys_ds_release(dmphys_dataspace_t * ds);

dmphys_dataspace_t *
dmphys_ds_get(l4_uint32_t ds_id);

int
dmphys_ds_get_check_owner(l4_uint32_t ds_id, 
			  l4_threadid_t caller,
			  dmphys_dataspace_t ** ds);

int
dmphys_ds_get_check_client(l4_uint32_t ds_id, 
			   l4_threadid_t caller,
			   dmphys_dataspace_t ** ds);

int
dmphys_ds_get_check_rights(l4_uint32_t ds_id, 
			   l4_threadid_t caller, 
			   l4_uint32_t rights, 
			   dmphys_dataspace_t ** ds);

void
dmphys_ds_iterate(dmphys_ds_iter_fn_t fn, 
		  void * data, 
		  l4_threadid_t owner, 
		  l4_uint32_t flags);

L4_INLINE l4_uint32_t
dmphys_ds_get_id(dmphys_dataspace_t * ds);

L4_INLINE void
dmphys_ds_add_pages(dmphys_dataspace_t * ds, 
		    page_area_t * pages, 
		    page_pool_t * pool);

L4_INLINE page_area_t * 
dmphys_ds_get_pages(dmphys_dataspace_t * ds);

L4_INLINE page_pool_t *
dmphys_ds_get_pool(dmphys_dataspace_t * ds);

L4_INLINE l4_size_t
dmphys_ds_get_size(dmphys_dataspace_t * ds);

L4_INLINE void
dmphys_ds_set_size(dmphys_dataspace_t * ds);

L4_INLINE char *
dmphys_ds_get_name(dmphys_dataspace_t * ds);

L4_INLINE void
dmphys_ds_set_name(dmphys_dataspace_t * ds, 
		   const char * name);

L4_INLINE page_area_t *
dmphys_ds_find_page_area(dmphys_dataspace_t * ds, 
			 l4_offs_t offset, 
			 l4_offs_t * area_offset);

L4_INLINE int
dmphys_ds_get_num_page_areas(dmphys_dataspace_t * ds);

L4_INLINE int
dmphys_ds_add_client(dmphys_dataspace_t * ds, 
		     l4_threadid_t client,
		     l4_uint32_t rights);

L4_INLINE int
dmphys_ds_remove_client(dmphys_dataspace_t * ds, 
			l4_threadid_t client);

L4_INLINE l4_uint32_t
dmphys_ds_get_rights(dmphys_dataspace_t * ds, 
		     l4_threadid_t client);

L4_INLINE int
dmphys_ds_set_rights(dmphys_dataspace_t * ds, 
		     l4_threadid_t client,
		     l4_uint32_t rights);

L4_INLINE void
dmphys_ds_set_owner(dmphys_dataspace_t * ds, 
		    l4_threadid_t owner);

int
dmphys_ds_count(l4_threadid_t owner, 
		l4_uint32_t flags);

void
dmphys_ds_show(dmphys_dataspace_t * ds);

/*****************************************************************************
 *** implementation
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Return dataspace id
 * 
 * \param  ds            Dataspace descriptor
 *	
 * \return Dataspace id.
 */
/*****************************************************************************/ 
L4_INLINE l4_uint32_t
dmphys_ds_get_id(dmphys_dataspace_t * ds)
{
  ASSERT(ds->desc != NULL);
  return dsmlib_get_id(ds->desc);
}

/*****************************************************************************/
/**
 * \brief  Add page list to dataspace descriptor
 * 
 * \param  ds            Dataspace descriptor
 * \param  pages         Page area list
 * \param  pool          Page pool
 */
/*****************************************************************************/ 
L4_INLINE void
dmphys_ds_add_pages(dmphys_dataspace_t * ds, 
		    page_area_t * pages, 
		    page_pool_t * pool)
{
  ds->pages = pages;
  ds->pool = pool;
  ds->size = dmphys_pages_get_size(pages);
}

/*****************************************************************************/
/**
 * \brief Return page list
 * 
 * \param  ds            Dataspace descriptor
 *
 * \return Pointer to page list head.
 */
/*****************************************************************************/ 
L4_INLINE page_area_t * 
dmphys_ds_get_pages(dmphys_dataspace_t * ds)
{
  ASSERT(ds->pages != NULL);
  return ds->pages;
}

/*****************************************************************************/
/**
 * \brief Return page pool used to allocate pages
 * 
 * \param  ds            Dataspace descriptor
 *	
 * \return Page pool descriptor.
 */
/*****************************************************************************/ 
L4_INLINE page_pool_t *
dmphys_ds_get_pool(dmphys_dataspace_t * ds)
{
  ASSERT(ds->pool != NULL);
  return ds->pool;
}

/*****************************************************************************/
/**
 * \brief Return dataspace size.
 * 
 * \param  ds            Dataspace descriptor
 *	
 * \return Dataspace size.
 */
/*****************************************************************************/ 
L4_INLINE l4_size_t
dmphys_ds_get_size(dmphys_dataspace_t * ds)
{
  return ds->size;
}

/*****************************************************************************/
/**
 * \brief  Recalculate dataspace size
 * 
 * \param  ds            Dataspace descriptor
 */
/*****************************************************************************/ 
L4_INLINE void
dmphys_ds_set_size(dmphys_dataspace_t * ds)
{
  ASSERT(ds->pages);
  ds->size = dmphys_pages_get_size(ds->pages);
}

/*****************************************************************************/
/**
 * \brief  Return dataspace name
 * 
 * \param  ds            Dataspace descriptor
 *	
 * \return Pointer to dataspace name.
 */
/*****************************************************************************/ 
L4_INLINE char *
dmphys_ds_get_name(dmphys_dataspace_t * ds)
{
  return dsmlib_get_name(ds->desc);
}

/*****************************************************************************/
/**
 * \brief  Set dataspace name
 * 
 * \param  ds            Dataspace descriptor
 * \param  name          Dataspace name
 */
/*****************************************************************************/ 
L4_INLINE void
dmphys_ds_set_name(dmphys_dataspace_t * ds, 
		   const char * name)
{
  dsmlib_set_name(ds->desc,name);
}

/*****************************************************************************/
/**
 * \brief  Find dataspace page area which contains offset
 * 
 * \param  list          Page area list head
 * \param  offset        Offset
 * \retval area_offset   Offset in page area
 *	
 * \return Pointer to page area, NULL if \a offset points beyond the end of 
 *         the dataspace.
 */
/*****************************************************************************/ 
L4_INLINE page_area_t *
dmphys_ds_find_page_area(dmphys_dataspace_t * ds, 
			 l4_offs_t offset, 
			 l4_offs_t * area_offset)
{
  ASSERT(ds->pages != NULL);
  return dmphys_pages_find_offset(ds->pages,offset,area_offset);
}

/*****************************************************************************/
/**
 * \brief  Return number of page areas allocated for dataspace
 * 
 * \param  ds            Dataspace descriptor
 *	
 * \return Number of page areas.
 */
/*****************************************************************************/ 
L4_INLINE int
dmphys_ds_get_num_page_areas(dmphys_dataspace_t * ds)
{
  ASSERT(ds->pages != NULL);
  return dmphys_pages_get_num(ds->pages);
}

/*****************************************************************************/
/**
 * \brief  Add client to dataspace.
 * 
 * \param  ds            Dataspace descriptor
 * \param  client        Client thread id
 * \param  rights        Cliient access rights 
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EINVAL  invalid dataspace descriptor
 *         - \c -L4_ENOMEM  out of memory allocating client descriptor
 */
/*****************************************************************************/ 
L4_INLINE int
dmphys_ds_add_client(dmphys_dataspace_t * ds, 
		     l4_threadid_t client,
		     l4_uint32_t rights)
{
  return dsmlib_add_client(ds->desc,client,rights);
}

/*****************************************************************************/
/**
 * \brief  Remove client of a dataspace
 * 
 * \param  ds            Dataspace descriptor
 * \param  client        Client thread id
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EINVAL     invalid dataspace descriptor
 *         - \c -L4_ENOTFOUND  client id not found
 */
/*****************************************************************************/ 
L4_INLINE int
dmphys_ds_remove_client(dmphys_dataspace_t * ds, 
			l4_threadid_t client)
{
  return dsmlib_remove_client(ds->desc,client);
}

/*****************************************************************************/
/**
 * \brief  Get access rights of a dataspace client
 * 
 * \param  ds            Dataspace descriptor
 * \param  client        Client thread id
 *	
 * \return Access rights
 */
/*****************************************************************************/ 
L4_INLINE l4_uint32_t
dmphys_ds_get_rights(dmphys_dataspace_t * ds, 
		     l4_threadid_t client)
{
  return dsmlib_get_rights(ds->desc,client);
}

/*****************************************************************************/
/**
 * \brief  Set dataspace access rights
 * 
 * \param  ds            Dataspace descriptor
 * \param  client        Client thread id
 * \param  rights        New access rights
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EINVAL     invlaid datatspace descriptor 
 *         - \c -L4_ENOTFOUND  client id not found
 */
/*****************************************************************************/ 
L4_INLINE int
dmphys_ds_set_rights(dmphys_dataspace_t * ds, 
		     l4_threadid_t client,
		     l4_uint32_t rights)
{
  return dsmlib_set_rights(ds->desc,client,rights);
}


/*****************************************************************************/
/**
 * \brief  Set dataspace owner
 * 
 * \param  ds            Dataspace descriptor
 * \param  owner         New dataspace owner
 */
/*****************************************************************************/ 
L4_INLINE void
dmphys_ds_set_owner(dmphys_dataspace_t * ds, 
		    l4_threadid_t owner)
{
  dsmlib_set_owner(ds->desc,owner);
}

#endif /* !_DM_PHYS___DATASPACE_H */
