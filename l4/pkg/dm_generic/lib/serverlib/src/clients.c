/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_generic/server-lib/src/clients.c
 * \brief  Generic dataspace manager library, client handling
 *
 * \date   11/21/2001
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

/* DM_generic includes */
#include <l4/dm_generic/dsmlib.h>
#include "__desc_alloc.h"
#include "__debug.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Find client descriptor
 *
 * \param  ds            Dataspace descriptor
 * \param  client        Client thread id
 *
 * \return Pointer to client descriptor, NULL if client not found.
 */
/*****************************************************************************/
static inline dsmlib_client_desc_t *
__find_client(const dsmlib_ds_desc_t * ds, l4_threadid_t client)
{
  dsmlib_client_desc_t * c = ds->clients;

  while ((c != NULL) && !l4_task_equal(client, c->client))
    c = c->next;

  return c;
}

/*****************************************************************************
 *** DSMlib API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Add client to dataspace
 *
 * \param  ds            Dataspace descriptor
 * \param  client        Client thread id
 * \param  rights        Rights bit mask
 *
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL  invalid dataspace descriptor
 *         - -#L4_ENOMEM  out of memory allocating client descriptor
 *
 * Add client to the client list of the dataspace. If the client already
 * exists, the rights are added to the client descriptor.
 */
/*****************************************************************************/
int
dsmlib_add_client(dsmlib_ds_desc_t * ds, l4_threadid_t client,
                  l4_uint32_t rights)
{
  dsmlib_client_desc_t * c;

  if (ds == NULL)
    return -L4_EINVAL;

  /* try to find client */
  c = __find_client(ds, client);
  if (c == NULL)
    {
      /* add new client descriptor */
      c = dsmlib_alloc_client_desc();
      if (c == NULL)
        {
          LOGdL(DEBUG_ERRORS, "DSMlib: client descriptor allocation failed!");
          return -L4_ENOMEM;
        }

      c->client = client;
      c->rights = rights;

      c->next = ds->clients;
      ds->clients = c;
    }
  else
    {
      /* add rights */
      c->rights |= rights;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Remove dataspace client
 *
 * \param  ds            Dataspace descriptor
 * \param  client        Client thread id
 *
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL     invalid dataspace descriptor
 *         - -#L4_ENOTFOUND  client id not found
 *
 * Remove client from the client list of the dataspace.
 */
/*****************************************************************************/
int
dsmlib_remove_client(dsmlib_ds_desc_t * ds, l4_threadid_t client)
{
  dsmlib_client_desc_t * c, * tmp;

  if (ds == NULL)
    return -L4_EINVAL;

  c = ds->clients;
  tmp = NULL;

  while ((c != NULL) && !l4_task_equal(client, c->client))
    {
      tmp = c;
      c = c->next;
    }

  if (c == NULL)
    return -L4_ENOTFOUND;

  /* remove client descriptor */
  if (tmp == NULL)
    ds->clients = c->next;
  else
    tmp->next = c->next;

  /* release client descriptor */
  dsmlib_free_client_desc(c);

  return 0;
}

/*****************************************************************************/
/**
 * \brief Remove all clients from dataspace
 *
 * \param  ds            Dataspace descriptor
 *
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL     invalid dataspace descriptor
 */
/*****************************************************************************/
int
dsmlib_remove_all_clients(dsmlib_ds_desc_t * ds)
{
  dsmlib_client_desc_t * tmp;

  if (ds == NULL)
    return -L4_EINVAL;

  while (ds->clients != NULL)
    {
      tmp = ds->clients;
      ds->clients = ds->clients->next;
      dsmlib_free_client_desc(tmp);
    }

  return 0;
}

/*****************************************************************************/
/**
 * \brief Check if someone is a client of a dataspace
 *
 * \param  ds            Dataspace descriptor
 * \param  client        Client thread id
 *
 * \return != 0 if \a client is a client of the dataspace, 0 otherwise.
 *
 * \note This only checks the client list of the dataspace, not if \a client
 *       is the owner of the dataspace.
 */
/*****************************************************************************/
int
dsmlib_is_client(const dsmlib_ds_desc_t * ds, l4_threadid_t client)
{
  if (ds == NULL)
    return 0;

  /* find client */
  if (__find_client(ds, client) == NULL)
    return 0;
  else
    return 1;
}

/*****************************************************************************/
/**
 * \brief Set client rights
 *
 * \param  ds            Dataspace descriptor
 * \param  client        Client thread id
 * \param  rights        Rights bit mask
 *
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL     invlaid datatspace descriptor
 *         - -#L4_ENOTFOUND  client id not found
 *
 * Set rights for \a client to \a rights.
 */
/*****************************************************************************/
int
dsmlib_set_rights(const dsmlib_ds_desc_t * ds, l4_threadid_t client,
                  l4_uint32_t rights)
{
  dsmlib_client_desc_t * c;

  if (ds == NULL)
    return -L4_EINVAL;

  /* find client */
  c = __find_client(ds, client);
  if (c == NULL)
    return -L4_ENOTFOUND;
  else
    c->rights = rights;

  return 0;
}

/*****************************************************************************/
/**
 * \brief Return client rights
 *
 * \param  ds            Dataspace descriptor
 * \param  client        Client thread id
 *
 * \return Rights bit mask, 0 if invlaid dataspace descriptor of client id
 *         not found
 *
 * Return rights bit mask for \a client.
 *
 * \note This function only returns the rights of the client stored in the
 *       client list. It does not check if the client is the owner of the
 *       dataspace.
 */
/*****************************************************************************/
l4_uint32_t
dsmlib_get_rights(const dsmlib_ds_desc_t * ds, l4_threadid_t client)
{
  dsmlib_client_desc_t * c;

  if (ds == NULL)
    return 0;

  /* find client */
  c = __find_client(ds, client);
  if (c == NULL)
    return 0;
  else
    return c->rights;
}

/*****************************************************************************/
/**
 * \brief Check client rights
 *
 * \param  ds            Dataspace descriptor
 * \param  client        Client thread id
 * \param  rights        Rights bit mask
 *
 * \return 1 if the client is allowed to perform the requested operations,
 *         0 otherwise
 *
 * \note This function only checks the rights of the client stored in the
 *       client list. It does not check if the client is the owner of the
 *       dataspace.
 */
/*****************************************************************************/
int
dsmlib_check_rights(const dsmlib_ds_desc_t * ds, l4_threadid_t client,
                    l4_uint32_t rights)
{
  dsmlib_client_desc_t * c;

  if (ds == NULL)
    return 0;

  /* find client */
  c = __find_client(ds, client);
  if (c == NULL)
    return 0;
  else
    return ((c->rights & rights) == rights);
}

/*****************************************************************************
 *** DEBUG
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  List dataspace clients
 *
 * \param  ds            Dataspace descriptor
 */
/*****************************************************************************/
void
dsmlib_list_ds_clients(const dsmlib_ds_desc_t * ds)
{
  dsmlib_client_desc_t * pc;

  if (ds == NULL)
    return;


  pc = ds->clients;
  while (pc != NULL)
    {
      LOG_printf(l4util_idfmt" (0x%x) ", l4util_idstr(pc->client), pc->rights);
      pc = pc->next;
    }
}
