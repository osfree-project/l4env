/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4env/lib/src/environment.c
 * \brief  Request environmen configuration
 *
 * \date   08/29/2000
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

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/env/env.h>
#include <l4/names/libnames.h>
#include "__config.h"

/*****************************************************************************
 *** Global data
 *****************************************************************************/

/**
 * Default dataspace manager, initially read from the environment page (or
 * set to a default value if the environment page is not available), it
 * can be replaced by l4env_set_default_dsm().
 */
static l4_threadid_t dsm_id = L4_INVALID_ID;

/**
 * Sigma0 id, set by the startup code
 */
static l4_threadid_t sigma0_id = L4_INVALID_ID;

/*****************************************************************************/
/**
 * \brief  Request id of environment service.
 *
 * \param  key           service key (see l4/env/env.h)
 * \retval service       service id
 *
 * \return 0 on success (\a service contains the requested service id),
 *         error code otherwise:
 *         - -L4_EINVAL   invlid key
 *         - -L4_ENODATA  value not set
 */
/*****************************************************************************/
int
l4env_request_service(l4_uint32_t key,
		      l4_threadid_t * service)
{
  l4env_infopage_t * infopage = l4env_get_infopage();

  /* get service id */
  *service = L4_INVALID_ID;
  switch(key)
    {
    case L4ENV_MEMORY_SERVER:
      /* anonymous memory server */
      if ((infopage != NULL) && (!l4_is_invalid_id(infopage->memserv_id)))
	*service = infopage->memserv_id;
      else
	return -L4_ENODATA;
      break;

    case L4ENV_TASK_SERVER:
      /* task server */
      if ((infopage != NULL) && (!l4_is_invalid_id(infopage->taskserv_id)))
	*service = infopage->taskserv_id;
      else
	return -L4_ENODATA;
      break;

    case L4ENV_NAME_SERVER:
      /* name server */
      if ((infopage != NULL) && (!l4_is_invalid_id(infopage->names_id)))
	*service = infopage->names_id;
      else
	return -L4_ENODATA;
      break;

    case L4ENV_FPROV_SERVER:
      /* file provider */
      if ((infopage != NULL) && (!l4_is_invalid_id(infopage->fprov_id)))
	*service = infopage->fprov_id;
      else
	return -L4_ENODATA;
      break;

    case L4ENV_SIGMA0:
      /* sigma0 */
      *service = sigma0_id;
      break;

    default:
      /* invalid service key */
      return -L4_EINVAL;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Request configuration dword.
 *
 * \param  key           config key
 * \retval cfg           configuration value
 *
 * \return 0 on success (\a cfg contains the confguratiuo value),
 *         error code otherwise:
 *         - -L4_EINVAL   invalid key
 *         - -L4_ENODATA  value not set
 */
/*****************************************************************************/
int
l4env_request_config_u32(l4_uint32_t key,
			 l4_addr_t * cfg)
{
  l4env_infopage_t * infopage = l4env_get_infopage();

  /* get config */
  switch(key)
    {
    case L4ENV_MAX_THREADS:
      /* max. number of threads */
      return -L4_ENODATA;

    case L4ENV_DEFAULT_STACK_SIZE:
      /* default size of the stack of a new thread */
      return -L4_ENODATA;

    case L4ENV_MAX_STACK_SIZE:
      /* max. size of a stack */
      return -L4_ENODATA;

    case L4ENV_DEFAULT_PRIO:
      /* default priority for new threads */
      return -L4_ENODATA;

    case L4ENV_VM_LOW:
      /* virtual memory start address */
      if ((infopage != NULL) && (infopage->vm_low != -1))
	*cfg = infopage->vm_low;
      else
	return -L4_ENODATA;
      break;

    case L4ENV_VM_HIGH:
      /* virtual memory end address */
      if ((infopage != NULL) && (infopage->vm_high != -1))
	*cfg = infopage->vm_high;
      else
	return -L4_ENODATA;
      break;

    default:
      /* invalid key */
      return -L4_EINVAL;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Request configuration string.
 *
 * \param  key           config key
 * \param  str           destination string buffer
 * \param  max_len       length of destination buffer
 *
 * \return 0 on succes (\a str contains the requested configuration string),
 *         error code otherwise:
 *         - -L4_EINVAL   invalid key
 *         - -L4_ENODATA  value not set
 */
/*****************************************************************************/
int
l4env_request_config_string(l4_uint32_t key,
			    char *str,
			    int max_len)
{
  return -L4_EINVAL;
}

/*****************************************************************************/
/**
 * \brief  Set sigma0 Id
 *
 * \param  id            Sigma0 id
 *
 * Set sigma0 id (used by startup code).
 */
/*****************************************************************************/
void
l4env_set_sigma0_id(l4_threadid_t id)
{
  sigma0_id = id;
}

/*****************************************************************************/
/**
 * \brief  Set default dataspace manager
 *
 * \param  id            Dataspace manager id
 */
/*****************************************************************************/
void
l4env_set_default_dsm(l4_threadid_t id)
{
  dsm_id = id;
}

/*****************************************************************************/
/**
 * \brief  Return default dataspace manager id
 *
 * \return Dataspace manager id, L4_INVALID_ID if no dataspace manager found
 */
/*****************************************************************************/
l4_threadid_t
l4env_get_default_dsm(void)
{
  l4env_infopage_t * infopage = l4env_get_infopage();

  if (l4_is_invalid_id(dsm_id))
    {
      if (infopage && !l4_is_invalid_id(infopage->memserv_id))
	dsm_id = infopage->memserv_id;
      else
	{
	  /* find default dataspace manager */
	  if (!names_waitfor_name(L4ENV_DEFAULT_DSM_NAME, &dsm_id, 60000))
	    dsm_id = L4_INVALID_ID;
	}
    }

  /* return dataspace manager id */
  return dsm_id;
}
