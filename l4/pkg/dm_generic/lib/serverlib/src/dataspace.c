/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/server-lib/src/dataspace.c
 * \brief  Dataspace manager library, dataspace management
 *
 * \date   11/21/2001
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

/* standard includes */
#include <string.h>

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* DM_generic includes */
#include <l4/dm_generic/consts.h>
#include <l4/dm_generic/dsmlib.h>
#include "__desc_alloc.h"
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** global data
 *****************************************************************************/

/**
 * Dataspace descriptor hash table
 */
static dsmlib_ds_desc_t * dataspaces[DSMLIB_DS_HASH];

/**
 * Dataspace list
 */
static dsmlib_ds_desc_t * ds_list = NULL;

/**
 * Last dataspace id used
 */
static l4_uint32_t last_id = 0;

/**
 * Hash function
 */
#define HASH_IDX(a)  (a % DSMLIB_DS_HASH)

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Find dataspace descriptor
 * 
 * \param  id            Dataspace id
 *	
 * \return Pointer to dataspace descriptor, NULL if not found.
 */
/*****************************************************************************/ 
static inline dsmlib_ds_desc_t *
__hash_find_ds(l4_uint32_t id)
{
  int hash_idx = HASH_IDX(id);
  dsmlib_ds_desc_t * tmp;

#if DEBUG_HASH
  INFO("id = %u, hash_idx = %d\n",id,hash_idx);
#endif

  tmp = dataspaces[hash_idx];
  while ((tmp != NULL) && (tmp->id < id))
    tmp = tmp->next;

  if ((tmp == NULL) || (tmp->id != id))
    return NULL;

  return tmp;
}

/*****************************************************************************/
/**
 * \brief Add dataspace to hash table
 * 
 * \param  ds            Dataspace descriptor
 *	
 * \return 0 on success, -1 if dataspace already exists
 */
/*****************************************************************************/ 
static int
__hash_add_dataspace(dsmlib_ds_desc_t * ds)
{
  int idx = HASH_IDX(ds->id);
  dsmlib_ds_desc_t * d, * tmp;

  ds->next = NULL;
  if (dataspaces[idx] == NULL)
    dataspaces[idx] = ds;
  else
    {
      d = dataspaces[idx];
      tmp = NULL;
      while ((d != NULL) && (d->id < ds->id))
	{
	  tmp = d;
	  d = d->next;
	}
      
      if (d == NULL)
	/* last element */
	tmp->next = ds;
      else if (d->id == ds->id)
	{
	  /* dataspace id already exists */
	  ERROR("DSMlib: dataspace %u already exists",ds->id);
	  return -1;
	}
      else if (tmp == NULL)
	{
	  /* first element */
	  ds->next = dataspaces[idx];
	  dataspaces[idx] = ds;
	}
      else
	{
	  ds->next = d;
	  tmp->next = ds;
	}
    }

  return 0;
}
	
/*****************************************************************************/
/**
 * \brief Remove dataspace from hash table
 * 
 * \param  ds            Dataspace descriptor
 *	
 * \return 0 on success, -1 if dataspace not found
 */
/*****************************************************************************/ 
static int
__hash_remove_dataspace(dsmlib_ds_desc_t * ds)
{	
  int idx = HASH_IDX(ds->id);
  dsmlib_ds_desc_t * d, * tmp;

  if (dataspaces[idx] == NULL)
    return -1;

  d = dataspaces[idx];
  tmp = NULL;
  while ((d != NULL) && (d->id != ds->id))
    {
      tmp = d;
      d = d->next;
    }
  
  if (d == NULL)
    {
      /* dataspace not found */
      ERROR("DSMlib: dataspace %u not found",ds->id);
      return -1;
    }

  if (tmp == NULL)
    /* remove first element in list */
    dataspaces[idx] = dataspaces[idx]->next;
  else
    tmp->next = d->next;
  
  return 0;
}

/*****************************************************************************/
/**
 * \brief Add dataspace to dataspace list 
 * 
 * \param  ds            Dataspace descriptor
 *
 * This function just adds the dataspace at the beginning of the dataspace 
 * list, it does not check whether the dataspace id already exists. Use it
 * together with __hash_add_dataspace()!
 */
/*****************************************************************************/ 
static inline void
__list_add_dataspace(dsmlib_ds_desc_t * ds)
{
  ds->ds_prev = NULL;
  ds->ds_next = ds_list;
  if (ds_list != NULL)
    ds_list->ds_prev = ds;
  ds_list = ds;
}

/*****************************************************************************/
/**
 * \brief Remove dataspace from dataspace list
 * 
 * \param  ds            Dataspace descriptor
 */
/*****************************************************************************/ 
static inline void 
__list_remove_dataspace(dsmlib_ds_desc_t * ds)
{
  if (ds->ds_prev == NULL)
    {
      ds_list = ds_list->ds_next;
      if (ds_list != NULL)
	ds_list->ds_prev = NULL;
    }
  else 
    {
      ds->ds_prev->ds_next = ds->ds_next;
      if (ds->ds_next != NULL)
	ds->ds_next->ds_prev = ds->ds_prev;
    }
}
    
/*****************************************************************************/
/**
 * \brief Find unused dataspace id
 *
 * \retval  id           Dataspace id
 *
 * \return 0 on success, -1 if no id found.
 */
/*****************************************************************************/ 
static inline int
__find_id(l4_uint32_t * id)
{
  l4_uint32_t i = last_id + 1;

  while ((i != last_id) && (__hash_find_ds(i) != NULL))
    i++;

  if (i == last_id)
    return -1;

#if DEBUG_DS_ID
  INFO("using id %u\n",i);
#endif

  *id = i;
  last_id = i;

  return 0;
}

/*****************************************************************************
 *** DSMlib API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Init dataspace manager library
 * 
 * \param  get_page_fn   Page allocation callback function
 * \param  free_page_fn  Page release callback function
 *	
 * \return 0 on succes, -1 if initialization failed
 */
/*****************************************************************************/ 
int
dsmlib_init(dsmlib_get_page_fn_t get_page_fn, 
	    dsmlib_free_page_fn_t free_page_fn)
{
  int i;

  /* initialize descriptor allocation */
  if (dsmlib_init_desc_alloc(get_page_fn,free_page_fn) < 0)
    {
      ERROR("DSMlib: descriptor allocation initialization failed!");
      return -1;
    }

  /* init dataspace hash table */
  for (i = 0; i < DSMLIB_DS_HASH; i++)
    dataspaces[i] = NULL;

  return 0;
}

/*****************************************************************************/
/**
 * \brief Create new dataspace
 *	
 * \return Pointer to new dataspace descriptor, NULL if creation failed.
 *
 * Allocate new dataspace descriptor, find unused dataspace id and add 
 * descriptor to hash table / dataspace list.
 */
/*****************************************************************************/ 
dsmlib_ds_desc_t *
dsmlib_create_dataspace(void)
{
  dsmlib_ds_desc_t * ds;
  l4_uint32_t id;
  
  ds = dsmlib_alloc_ds_desc();
  if (ds == NULL)
    {
      ERROR("DSMlib: dataspace descriptor allocation failed!");
      return NULL;
    }

  if (__find_id(&id) < 0)
    {
      ERROR("DSMlib: no dataspace id available!");
      dsmlib_free_ds_desc(ds);
      return NULL;
    }

  /* setup dataspace descriptor */
  ds->id = id;
  ds->name[0] = 0;
  ds->owner = L4_INVALID_ID;
  ds->clients = NULL;
  ds->dsm_ptr = NULL;
  ds->next = NULL;
  ds->ds_next = NULL;
  ds->ds_prev = NULL;

  if (__hash_add_dataspace(ds) < 0)
    {
      ERROR("DSMlib: add dataspace %u to hash table failed!",ds->id);
      dsmlib_free_ds_desc(ds);
      return NULL;
    }

  __list_add_dataspace(ds);

  /* done */
  return ds;
}

/*****************************************************************************/
/**
 * \brief Release dataspace
 * 
 * \param  ds            Dataspace descriptor
 *
 * Remove dataspace from hash table, release dataspace descriptor
 */
/*****************************************************************************/ 
void
dsmlib_release_dataspace(dsmlib_ds_desc_t * ds)
{
  /* remove dataspace from hash table */
  if (__hash_remove_dataspace(ds) < 0)
    {
      ERROR("DSMlib: remove dataspace %u from hash table failed!",ds->id);
      return;
    }

  /* remove dataspace from dataspace list */
  __list_remove_dataspace(ds);

  /* release all client descriptors */
  dsmlib_remove_all_clients(ds);
  
  /* done */
  dsmlib_free_ds_desc(ds);
}

/*****************************************************************************/
/**
 * \brief Return dataspace descriptor
 * 
 * \param  id            Dataspace id
 *	
 * \return Pointer to dataspace descriptor, NULL if dataspace does not exist
 */
/*****************************************************************************/ 
dsmlib_ds_desc_t *
dsmlib_get_dataspace(l4_uint32_t id)
{
  return __hash_find_ds(id);
}

/*****************************************************************************/
/**
 * \brief Set dataspace owner
 * 
 * \param  ds            Dataspace descriptor
 * \param  owner         Dataspace owner
 */
/*****************************************************************************/ 
void
dsmlib_set_owner(dsmlib_ds_desc_t * ds, 
		 l4_threadid_t owner)
{
  if (ds == NULL)
    return;

  ds->owner = owner;
}

/*****************************************************************************/
/**
 * \brief Return owner of the dataspace
 * 
 * \param  ds            Dataspace descriptor
 *	
 * \return owner thread id, \c L4_INVALID_ID if invalid dataspace id
 */
/*****************************************************************************/ 
l4_threadid_t 
dsmlib_get_owner(dsmlib_ds_desc_t * ds)
{
  if (ds == NULL)
    return L4_INVALID_ID;

  return ds->owner;
}

/*****************************************************************************/
/**
 * \brief Check owner of dataspace
 * 
 * \param  ds            Dataspace descriptor
 * \param  client        Client thread id
 *	
 * \return 1 if the client owns the dataspace, 0 otherwise.
 */
/*****************************************************************************/ 
int
dsmlib_is_owner(dsmlib_ds_desc_t * ds, 
		l4_threadid_t client)
{
  if (ds == NULL)
    return 0;

  return l4_thread_equal(ds->owner,client);
}

/*****************************************************************************/
/**
 * \brief Set dataspace name
 * 
 * \param  ds            Dataspace descriptor
 * \param  name          Dataspace name
 */
/*****************************************************************************/ 
void
dsmlib_set_name(dsmlib_ds_desc_t * ds, 
		const char * name)
{
  if (ds == NULL)
    return;

  if (name == NULL)
    ds->name[0] = 0;
  else
    {
      strncpy(ds->name,name,L4DM_DS_NAME_MAX_LEN);
      ds->name[L4DM_DS_NAME_MAX_LEN] = 0;
    }
}

/*****************************************************************************/
/**
 * \brief Get dataspace name
 * 
 * \param  ds            Dataspace descriptor
 *	
 * \return Pointer to dataspace name, NULL if invalid dataspace descriptor
 */
/*****************************************************************************/ 
char * 
dsmlib_get_name(dsmlib_ds_desc_t * ds)
{
  if (ds == NULL)
    return NULL;

  return ds->name;
}

/*****************************************************************************/
/**
 * \brief Set dataspace manager data
 * 
 * \param  ds            Dataspace descriptor
 * \param  ptr           Dataspace manager data
 */
/*****************************************************************************/ 
void
dsmlib_set_dsm_ptr(dsmlib_ds_desc_t * ds, 
		   void * ptr)
{
  if (ds == NULL)
    return;

  ds->dsm_ptr = ptr;
}

/*****************************************************************************/
/**
 * \brief Get dataspace manager data
 * 
 * \param  ds            Dataspace descriptor
 *	
 * \return Pointer to dataspace manager data
 */
/*****************************************************************************/ 
void *
dsmlib_get_dsm_ptr(dsmlib_ds_desc_t * ds)
{
  if (ds == NULL)
    return NULL;

  return ds->dsm_ptr;
}

/*****************************************************************************/
/**
 * \brief  Return dataspace list head
 * 
 * \return Pointer to dataspace list, NULL if list empty.
 */
/*****************************************************************************/ 
dsmlib_ds_desc_t *
dsmlib_get_dataspace_list(void)
{
  return ds_list;
}

/*****************************************************************************/
/**
 * \brief  Iterate dataspace list
 * 
 * \param  fn            Iterator function
 * \param  data          Iterator function data
 */
/*****************************************************************************/ 
void
dsmlib_dataspaces_iterate(dsmlib_iterator_fn_t fn, 
			  void * data)
{
  dsmlib_ds_desc_t * pd, * next;

  /* iterate */
  pd = ds_list;
  while (pd != NULL)
    {
      /* get next dataspace in list prior calling the iterate function,
       * this allows to remove the dataspace from the list by the 
       * iterate function (-> close)
       */
      next = pd->ds_next;

      /* call iterator function */
      fn(pd,data);

      pd = next;
    }
}

/*****************************************************************************
 *** DEBUG
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Show dataspace hash table
 */
/*****************************************************************************/ 
void
dsmlib_show_ds_hash(void)
{
  int i;
  dsmlib_ds_desc_t * d;

  Msg("Dataspace hash table:\n");
  for (i = 0; i < DSMLIB_DS_HASH; i++)
    {
      Msg("  Hash idx %d:\n   ",i);
      d = dataspaces[i];
      while (d != NULL)
	{
	  Msg(" %2u",d->id);
	  d = d->next;
	}
      Msg("\n");
    } 
}

/*****************************************************************************/
/**
 * \brief Lists dataspaces
 */
/*****************************************************************************/ 
void
dsmlib_list_ds(void)
{
  dsmlib_ds_desc_t * d; 

  Msg("Dataspace list:");
  d = ds_list;
  while (d != NULL)
    {
      Msg(" %2u",d->id);
      d = d->ds_next;
    }
  Msg("\n");
}
