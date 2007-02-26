/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/dataspace.c
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

/* standard includes */
#include <string.h>

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/slab/slab.h>
#include <l4/dm_generic/dsmlib.h>

/* DMphys includes */
#include "__dataspace.h"
#include "__internal_alloc.h"
#include "__debug.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/**
 * Dataspace descriptor slab cache 
 */
l4slab_cache_t dataspace_cache;

/**
 * Dataspace descriptor slab cache name
 */
static char * dataspace_cache_name = "dataspace";

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Page allocation function for dsmlib
 * 
 * \retval data          Pointer to page area structure 
 *	
 * \return Pointer to new page
 */
/*****************************************************************************/ 
static void *
__dsmlib_get_page(void ** data)
{
  /* allocate page */
  return dmphys_internal_allocate(data);
}

/*****************************************************************************/
/**
 * \brief Page frelease function for dsmlib
 * 
 * \param  page          Page address
 * \param  data          Page area address, set by dmphys_dsmlib_get_page
 */
/*****************************************************************************/ 
static void
__dsmlib_free_page(void * page, 
		   void * data)
{
  /* release page */
  dmphys_internal_release(page,data);
}

/*****************************************************************************
 *** DMphys internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Initialize dataspace descriptor allocation
 *	
 * \return 0 on succes, -1 if initialization failed.
 */
/*****************************************************************************/ 
int
dmphys_ds_init(void)
{
  /* init dataspace descriptor slab cache, 
   * use default grow/release functions */
  if (l4slab_cache_init(&dataspace_cache,sizeof(dmphys_dataspace_t),0,
			dmphys_internal_alloc_grow,
			dmphys_internal_alloc_release) < 0)
    return -1;

  l4slab_set_data(&dataspace_cache,dataspace_cache_name);

  /* initialize dataspace manager library */
  if (dsmlib_init(__dsmlib_get_page,__dsmlib_free_page) < 0)
    return -1;
  
  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Create new dataspace descriptor.
 * 
 * \param  owner         Dataspace owner
 * \param  name          Dataspace name
 * \param  flags         Flags
 *	
 * \return Pointer to dataspace descriptor, NULL if creation failed.
 */
/*****************************************************************************/ 
dmphys_dataspace_t *
dmphys_ds_create(l4_threadid_t owner, 
		 const char * name, 
		 l4_uint32_t flags)
{
  dmphys_dataspace_t * ds;
  dsmlib_ds_desc_t * desc;
  
  /* create internal dataspace descriptor */
  ds = l4slab_alloc(&dataspace_cache);
  if (ds == NULL)
    {
      ERROR("DMphys: allocating dataspace descriptor failed!");
      return NULL;
    }

  /* allocate global dataspace descriptor */
  desc = dsmlib_create_dataspace(); 
  if (desc == NULL) 
    {
      ERROR("DMphys: allocating global dataspace descriptor failed!");
      return NULL;
    }

  /* initialize descriptors */
  ds->desc = desc;
  ds->pages = NULL;
  ds->pool = NULL;
  ds->size = 0;
  ds->flags = flags;
  dsmlib_set_dsm_ptr(desc,ds);
  dsmlib_set_owner(desc,owner);
  dsmlib_set_name(desc,name);

  /* done */
  return ds;
}

/*****************************************************************************/
/**
 * \brief Release dataspace descriptor
 * 
 * \param  ds            Dataspace descriptor
 */
/*****************************************************************************/ 
void
dmphys_ds_release(dmphys_dataspace_t * ds)
{
  /* release global dataspace descriptor */
  dsmlib_release_dataspace(ds->desc);

  /* release internal dataspace descriptor */
  l4slab_free(&dataspace_cache,ds);
}

/*****************************************************************************/
/**
 * \brief Get dataspace descriptor for dataspace id
 * 
 * \param  ds_id         Dataspace id
 *	
 * \return Dataspace descriptor, NULL if dataspace id not exists.
 */
/*****************************************************************************/ 
dmphys_dataspace_t *
dmphys_ds_get(l4_uint32_t ds_id)
{
  dsmlib_ds_desc_t * desc = dsmlib_get_dataspace(ds_id);
  dmphys_dataspace_t * ds;

  if (desc == NULL)
    return NULL;

  ds = dsmlib_get_dsm_ptr(desc);
  ASSERT((ds != NULL) && (ds->desc == desc));
 
  return ds;
}

/*****************************************************************************/
/**
 * \brief  Get dataspace descriptor for dataspace id, check if caller is the
 *         owner
 * 
 * \param  ds_id         Dataspace id
 * \param  caller        Caller thread id
 * \retval ds            Dataspace descriptor.
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_EPERM   caller is not the owner of the dataspace
 */
/*****************************************************************************/ 
int
dmphys_ds_get_check_owner(l4_uint32_t ds_id, 
			  l4_threadid_t caller,
			  dmphys_dataspace_t ** ds)
{
  dsmlib_ds_desc_t * desc = dsmlib_get_dataspace(ds_id);
  
  *ds = NULL;
  if (desc == NULL)
    return -L4_EINVAL;

  *ds = dsmlib_get_dsm_ptr(desc);
  ASSERT((*ds != NULL) && ((*ds)->desc == desc));

  /* check owner */
  if (!dsmlib_is_owner(desc,caller))
    return -L4_EPERM;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Get dataspace descriptor for dataspace id, check if caller is a 
 *         client of the dataspace
 * 
 * \param  ds_id         Dataspace id
 * \param  caller        Caller thread id
 * \retval ds            Dataspace descriptor
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_EPERM   caller is not a client of the dataspace
 */
/*****************************************************************************/ 
int
dmphys_ds_get_check_client(l4_uint32_t ds_id, 
			   l4_threadid_t caller,
			   dmphys_dataspace_t ** ds)
{
  dsmlib_ds_desc_t * desc = dsmlib_get_dataspace(ds_id);
  
  *ds = NULL;
  if (desc == NULL)
    return -L4_EINVAL;

  *ds = dsmlib_get_dsm_ptr(desc);
  ASSERT((*ds != NULL) && ((*ds)->desc == desc));

  /* check if caller is a client or the owner of the dataspace */
  if (!dsmlib_is_owner(desc,caller) && !dsmlib_is_client(desc,caller))
    return -L4_EPERM;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Get dataspace descriptor for dataspace id, check if caller is 
 *         permitted to perform the requested operations.
 * 
 * \param  ds_id         Dataspace id
 * \param  caller        Caller thread id
 * \param  rights        Rights bit mask
 * \retval ds            Dataspace descriptor
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_EPERM   caller is not permitted to perform the requested 
 *                          operations         
 */
/*****************************************************************************/ 
int
dmphys_ds_get_check_rights(l4_uint32_t ds_id, 
			   l4_threadid_t caller, 
			   l4_uint32_t rights, 
			   dmphys_dataspace_t ** ds)
{
  dsmlib_ds_desc_t * desc = dsmlib_get_dataspace(ds_id);
  
  *ds = NULL;
  if (desc == NULL)
    return -L4_EINVAL;

  *ds = dsmlib_get_dsm_ptr(desc);
  ASSERT((*ds != NULL) && ((*ds)->desc == desc));

  /* check if caller has requested rights or is the owner of the dataspace */
  if (!dsmlib_check_rights(desc,caller,rights) && 
      !dsmlib_is_owner(desc,caller))
    return -L4_EPERM;

  /* done */
  return 0;
}

/*****************************************************************************
 *** DEBUG stuff
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Count dataspace iterator function
 * 
 * \param  ds            Dataspace descriptor
 * \param  data          Data pointer, counter
 */
/*****************************************************************************/ 
static void
__ds_count_iterator(dmphys_dataspace_t * ds, 
		    void * data)
{
  (*((int *)data))++;
}

/*****************************************************************************/
/**
 * \brief  Count dataspaces
 * 
 * \param  owner         Count dataspaces of owner, if set to L4_INVALID_ID
 *                       count all dataspaces.
 * \param  flags         Flags:
 *                       - #L4DM_SAME_TASK count dataspaces owned by task
 *
 * \return Number of dataspaces.
 */
/*****************************************************************************/ 
int
dmphys_ds_count(l4_threadid_t owner, 
		l4_uint32_t flags)
{
  int num = 0;

  /* count */
  dmphys_ds_iterate(__ds_count_iterator,&num,owner,flags);

  return num;
}

/*****************************************************************************/
/**
 * \brief  Show dataspace information
 * 
 * \param  ds            Dataspace descriptor
 */
/*****************************************************************************/ 
void
dmphys_ds_show(dmphys_dataspace_t * ds)
{
  l4_threadid_t ds_owner = dsmlib_get_owner(ds->desc);
  char * name = dsmlib_get_name(ds->desc);

  printf("DMphys dataspace %u:\n",dsmlib_get_id(ds->desc));
  if (strlen(name) > 0)
    printf("  name \"%s\"\n",name);
  printf("  owner %x.%x, size 0x%08x (%uKB)\n",ds_owner.id.task,
         ds_owner.id.lthread,ds->size,ds->size / 1024);
  printf("  clients: ");
  dsmlib_list_ds_clients(ds->desc);
  printf("\n");
  if (strlen(ds->pool->name) > 0)
    printf("  memory areas (pool %d, \"%s\"):\n",ds->pool->pool,ds->pool->name);
  else
    printf("  memory areas (pool %d):\n",ds->pool->pool);
  dmphys_pages_list(ds->pages);
}
