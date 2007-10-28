/* $Id$ */
/*****************************************************************************/
/**
 * \file    dm_generic/include/dsmlib.h
 * \brief   Dataspace Manager support library
 * \ingroup dsmlib
 *
 * \date    11/20/2001
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _L4_DM_GENERIC_DSMLIB_H
#define _L4_DM_GENERIC_DSMLIB_H

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/cdefs.h>

/* dm_generic includes */
#include <l4/dm_generic/consts.h>
#include <l4/dm_generic/types.h>

/*****************************************************************************
 *** datatypes
 *****************************************************************************/

/**
 * Dataspace client descriptor
 * \ingroup dsmlib_client
 */
typedef struct dsmlib_client_desc
{
  l4_threadid_t               client;  ///< client thread id
  l4_uint32_t                 rights;  ///< rights bit mask

  struct dsmlib_client_desc * next;    ///< next entry in client list
} dsmlib_client_desc_t;

/**
 * Generic dataspace descriptor
 * \ingroup dsmlib_ds
 */
typedef struct dsmlib_ds_desc
{
  l4_uint32_t             id;          ///< dataspace id
  char                    name[L4DM_DS_NAME_MAX_LEN + 1]; ///< dataspace name
  l4_threadid_t           owner;       ///< dataspace owner
  dsmlib_client_desc_t *  clients;     ///< client list
  void *                  dsm_ptr;     ///< dataspace manager data

  struct dsmlib_ds_desc * next;        ///< next element in hash list

  struct dsmlib_ds_desc * ds_next;     ///< next element in dataspace list
  struct dsmlib_ds_desc * ds_prev;     ///< previous element in dataspace list
} dsmlib_ds_desc_t;

/**
 * Dataspace list iterator function type
 * \ingroup dsmlib_iterate
 */
typedef void (* dsmlib_iterator_fn_t) (dsmlib_ds_desc_t * ds, void * data);

/**
 * Return dataspace id
 * \ingroup dsmlib_ds
 */
#define dsmlib_get_id(ds)     ((ds)->id)

/**
 * Next dataspace in dataspace list
 * \ingroup dsmlib_iterate
 */
#define dsmlib_next_ds(ds)    ((ds)->ds_next)

/**
 * Previous dataspace in dataspace list
 * \ingroup dsmlib_iterate
 */
#define dsmlib_prev_ds(ds)    ((ds)->ds_prev)

/*****************************************************************************
 *** Memory allocation
 *****************************************************************************/

/**
 * Page allocation callback function
 * \ingroup dsmlib_init
 *
 * This function is used by the dsmlib to allocate pages used for the
 * dataspace / client descriptors. The \a data pointer can be used to store
 * application data, it is returned with the page release callback function.
 */
typedef void * (* dsmlib_get_page_fn_t)(void ** data);

/**
 * Page release callback function
 * \ingroup dsmlib_init
 */
typedef void (* dsmlib_free_page_fn_t)(void * page, void * data);

/*****************************************************************************
 *** public API
 *****************************************************************************/

__BEGIN_DECLS;

/*****************************************************************************/
/**
 * \brief   Init dataspace manager library
 * \ingroup dsmlib_init
 *
 * \param   get_page_fn  Page allocation callback function
 * \param   free_page_fn Page release callback function
 *
 * \return  0 on succes, -1 if initialization failed
 */
/*****************************************************************************/
int
dsmlib_init(dsmlib_get_page_fn_t get_page_fn,
            dsmlib_free_page_fn_t free_page_fn);

/*****************************************************************************/
/**
 * \brief   Create new dataspace
 * \ingroup dsmlib_ds
 *
 * \return  Pointer to new dataspace descriptor, #NULL if creation failed.
 *
 * This function finds an unused dataspace id and creates a new dataspace
 * descriptor. The new descriptor is added to the dataspace hash table and
 * the dataspace list.
 */
/*****************************************************************************/
dsmlib_ds_desc_t *
dsmlib_create_dataspace(void);

/*****************************************************************************/
/**
 * \brief   Release dataspace
 * \ingroup dsmlib_ds
 *
 * \param   ds           Dataspace descriptor
 *
 * Removes the dataspace from the hash table / dataspace list and releases
 * the dataspace descriptor.
 */
/*****************************************************************************/
void
dsmlib_release_dataspace(dsmlib_ds_desc_t * ds);

/*****************************************************************************/
/**
 * \brief   Return dataspace descriptor
 * \ingroup dsmlib_ds
 *
 * \param   id           Dataspace id
 *
 * \return  Pointer to dataspace descriptor, #NULL if dataspace does not exist
 */
/*****************************************************************************/
dsmlib_ds_desc_t *
dsmlib_get_dataspace(l4_uint32_t id);

/*****************************************************************************/
/**
 * \brief   Set dataspace owner
 * \ingroup dsmlib_ds
 *
 * \param   ds           Dataspace descriptor
 * \param   owner        Dataspace owner
 */
/*****************************************************************************/
void
dsmlib_set_owner(dsmlib_ds_desc_t * ds, l4_threadid_t owner);

/*****************************************************************************/
/**
 * \brief   Return owner of the dataspace
 * \ingroup dsmlib_ds
 *
 * \param   ds           Dataspace descriptor
 *
 * \return  owner thread id, #L4_INVALID_ID if invalid dataspace id
 */
/*****************************************************************************/
l4_threadid_t
dsmlib_get_owner(const dsmlib_ds_desc_t * ds);

/*****************************************************************************/
/**
 * \brief   Check owner of dataspace
 * \ingroup dsmlib_ds
 *
 * \param   ds            Dataspace descriptor
 * \param   client        Client thread id
 *
 * \return  1 if the client owns the dataspace, 0 otherwise.
 */
/*****************************************************************************/
int
dsmlib_is_owner(const dsmlib_ds_desc_t * ds, l4_threadid_t client);

/*****************************************************************************/
/**
 * \brief   Set dataspace name
 * \ingroup dsmlib_ds
 *
 * \param   ds           Dataspace descriptor
 * \param   name         Dataspace name
 */
/*****************************************************************************/
void
dsmlib_set_name(dsmlib_ds_desc_t * ds, const char * name);

/*****************************************************************************/
/**
 * \brief   Get dataspace name
 * \ingroup dsmlib_ds
 *
 * \param   ds           Dataspace descriptor
 *
 * \return  Pointer to dataspace name, #NULL if invalid dataspace descriptor
 */
/*****************************************************************************/
char *
dsmlib_get_name(dsmlib_ds_desc_t * ds);

/*****************************************************************************/
/**
 * \brief   Set dataspace manager data
 * \ingroup dsmlib_ds
 *
 * \param   ds           Dataspace descriptor
 * \param   ptr          Dataspace manager data
 */
/*****************************************************************************/
void
dsmlib_set_dsm_ptr(dsmlib_ds_desc_t * ds, void * ptr);

/*****************************************************************************/
/**
 * \brief   Get dataspace manager data
 * \ingroup dsmlib_ds
 *
 * \param   ds           Dataspace descriptor
 *
 * \return Pointer to dataspace manager data
 */
/*****************************************************************************/
void *
dsmlib_get_dsm_ptr(const dsmlib_ds_desc_t * ds);

/*****************************************************************************/
/**
 * \brief   Add client to dataspace
 * \ingroup dsmlib_client
 *
 * \param   ds           Dataspace descriptor
 * \param   client       Client thread id
 * \param   rights       Rights bit mask (user defined)
 *
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL  invalid dataspace descriptor
 *          - -#L4_ENOMEM  out of memory allocating client descriptor
 *
 * Add client to the client list of the dataspace. If the client already
 * exists, the rights are added to the client descriptor.
 */
/*****************************************************************************/
int
dsmlib_add_client(dsmlib_ds_desc_t * ds, l4_threadid_t client,
                  l4_uint32_t rights);

/*****************************************************************************/
/**
 * \brief   Remove dataspace client
 * \ingroup dsmlib_client
 *
 * \param   ds           Dataspace descriptor
 * \param   client       Client thread id
 *
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL     invalid dataspace descriptor
 *          - -#L4_ENOTFOUND  client id not found
 *
 * Remove client from the client list of the dataspace.
 */
/*****************************************************************************/
int
dsmlib_remove_client(dsmlib_ds_desc_t * ds, l4_threadid_t client);

/*****************************************************************************/
/**
 * \brief   Remove all clients from dataspace
 * \ingroup dsmlib_client
 *
 * \param   ds           Dataspace descriptor
 *
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL     invalid dataspace descriptor
 */
/*****************************************************************************/
int
dsmlib_remove_all_clients(dsmlib_ds_desc_t * ds);

/*****************************************************************************/
/**
 * \brief   Check if someone is a client of a dataspace
 * \ingroup dsmlib_client
 *
 * \param   ds           Dataspace descriptor
 * \param   client       Client thread id
 *
 * \return  != 0 if \a client is a client of the dataspace, 0 otherwise.
 *
 * \note    This only checks the client list of the dataspace, not if
 *          \a client is the owner of the dataspace.
 */
/*****************************************************************************/
int
dsmlib_is_client(const dsmlib_ds_desc_t * ds, l4_threadid_t client);

/*****************************************************************************/
/**
 * \brief   Set client rights
 * \ingroup dsmlib_client
 *
 * \param   ds           Dataspace descriptor
 * \param   client       Client thread id
 * \param   rights       Rights bit mask (user defined)
 *
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL     invalid datatspace descriptor
 *          - -#L4_ENOTFOUND  client id not found
 *
 * Set rights for \a client to \a rights.
 */
/*****************************************************************************/
int
dsmlib_set_rights(const dsmlib_ds_desc_t * ds, l4_threadid_t client,
                  l4_uint32_t rights);

/*****************************************************************************/
/**
 * \brief   Return client rights
 * \ingroup dsmlib_client
 *
 * \param   ds           Dataspace descriptor
 * \param   client       Client thread id
 *
 * \return  Rights bit mask, 0 if invalid dataspace descriptor or client id
 *          not found
 *
 * Return rights bit mask for \a client.
 *
 * \note    This function only returns the rights of the client stored in the
 *          client list. It does not check if the client is the owner of the
 *          dataspace.
 */
/*****************************************************************************/
l4_uint32_t
dsmlib_get_rights(const dsmlib_ds_desc_t * ds, l4_threadid_t client);

/*****************************************************************************/
/**
 * \brief   Check client rights
 * \ingroup dsmlib_client
 *
 * \param   ds           Dataspace descriptor
 * \param   client       Client thread id
 * \param   rights       Rights bit mask
 *
 * \return  1 if the client is allowed to perform the requested operations,
 *          0 otherwise
 *
 * \note    This function only checks the rights of the client stored in the
 *          client list. It does not check if the client is the owner of the
 *          dataspace.
 */
/*****************************************************************************/
int
dsmlib_check_rights(const dsmlib_ds_desc_t * ds, l4_threadid_t client,
                    l4_uint32_t rights);

/*****************************************************************************/
/**
 * \brief   Return dataspace list head
 * \ingroup dsmlib_iterate
 *
 * \return  Pointer to dataspace list, #NULL if list empty.
 */
/*****************************************************************************/
dsmlib_ds_desc_t *
dsmlib_get_dataspace_list(void);

/*****************************************************************************/
/**
 * \brief   Iterate dataspace list
 * \ingroup dsmlib_iterate
 *
 * \param   fn           Iterator function
 * \param   data         Iterator function data
 */
/*****************************************************************************/
void
dsmlib_dataspaces_iterate(dsmlib_iterator_fn_t fn, void * data);

/*****************************************************************************
 *** DEBUG
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Show dataspace hash table
 * \ingroup dsmlib_debug
 */
/*****************************************************************************/
void
dsmlib_show_ds_hash(void);

/*****************************************************************************/
/**
 * \brief   List dataspaces
 * \ingroup dsmlib_debug
 */
/*****************************************************************************/
void
dsmlib_list_ds(void);

/*****************************************************************************/
/**
 * \brief   List dataspace clients
 * \ingroup dsmlib_debug
 *
 * \param   ds           Dataspace descriptor
 */
/*****************************************************************************/
void
dsmlib_list_ds_clients(const dsmlib_ds_desc_t * ds);

__END_DECLS;

#endif /* !_L4_DM_GENERIC_DSMLIB_H */
