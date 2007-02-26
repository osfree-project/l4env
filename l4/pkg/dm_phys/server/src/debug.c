/* $Id$ */
/*****************************************************************************/
/**
 * \file   dm_phys/server/src/debug.c
 * \brief  DMphys, debug functions
 *
 * \date   11/22/2001
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
#include <stdio.h>

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* DMphys includes */
#include "dm_phys-server.h"
#include <l4/dm_phys/consts.h>
#include "__dataspace.h"
#include "__dm_phys.h"
#include "__pages.h"
#include "__debug.h"

/*****************************************************************************
 *** typedefs
 *****************************************************************************/

/**
 * Dump iterator argument 
 */
typedef struct dump_arg
{
  l4_size_t   sum;
  l4_size_t   size;
  char *      str_buf;
} dump_arg_t;

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  List dataspaces iterator function
 * 
 * \param  ds            Dataspace descriptor
 * \param  data          Data pointer, total size of all dataspaces
 */
/*****************************************************************************/ 
static void
__ds_list_iterator(dmphys_dataspace_t * ds, 
		   void * data)
{
  l4_size_t * sum = data;
  l4_threadid_t ds_owner = dsmlib_get_owner(ds->desc);

  Msg("%4d: size=%08x  owner=%02x.%02x  name=\"%s\"\n",
      dmphys_ds_get_id(ds),ds->size,ds_owner.id.task,
      ds_owner.id.lthread,dmphys_ds_get_name(ds));
  *sum += ds->size;
}

/*****************************************************************************/
/**
 * \brief  Dump dataspaces iterator function
 * 
 * \param  ds            Dataspace descriptor
 * \param  data          Data pointer, dump argument buffer
 */
/*****************************************************************************/ 
static void
__ds_dump_iterator(dmphys_dataspace_t * ds, 
		   void * data)
{
  dump_arg_t * arg = data;
  l4_threadid_t ds_owner = dsmlib_get_owner(ds->desc);
  int printed;

  printed = snprintf(arg->str_buf, arg->size-1,
		     "%4d: size=%08x  owner=%02x.%02x  name=\"%s\"\n",
		     dmphys_ds_get_id(ds),ds->size,ds_owner.id.task,
		     ds_owner.id.lthread,dmphys_ds_get_name(ds));

  arg->str_buf += printed;
  arg->size    -= printed;
  arg->sum     += ds->size;
}

/*****************************************************************************
 *** DMphys IDL server functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Set dataspace name
 * 
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \param  name          Dataspace name
 * \retval _ev           Flick exception structure, unused
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EINVAL  invalid dataspace id
 *         - \c -L4_EPERM   caller is not the owner of the dataspace
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_memphys_server_set_name(sm_request_t * request, 
				l4_uint32_t ds_id, 
				const char * name, 
				sm_exc_t * _ev)
{
  int ret;
  dmphys_dataspace_t * ds;

  /* get dataspace descriptor */
  ret = dmphys_ds_get_check_owner(ds_id,request->client_tid,&ds);
  if (ret < 0)
    {
#if DEBUG_ERRORS
      if (ret == -L4_EINVAL)
	ERROR("DMphys: invalid dataspace id %u, caller %x.%x",ds_id,
	      request->client_tid.id.task,request->client_tid.id.lthread);
      else
	ERROR("DMphys: client %x.%x does not own dataspace %u",
	      request->client_tid.id.task,request->client_tid.id.lthread,
	      ds_id);
#endif      
      return ret;
    }

  /* set name */
  dmphys_ds_set_name(ds,name);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Return dataspace name
 * 
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \retval name          Dataspace name
 * \retval _ev           Flick exception structure, unused
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EINVAL  invalid dataspace id
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_memphys_server_get_name(sm_request_t * request, 
				l4_uint32_t ds_id, 
				char ** name, 
				sm_exc_t * _ev)
{
  dmphys_dataspace_t * ds;
  
  /* get dataspace descriptor */
  ds = dmphys_ds_get(ds_id);
  if (ds == NULL)
    {
      ERROR("DMphys: invalid dataspace %u, caller %x.%x!",ds_id,
	    request->client_tid.id.task,request->client_tid.id.lthread);
      return -L4_EINVAL;
    }

  /* return name */
  *name = dmphys_ds_get_name(ds);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Show dataspace information
 * 
 * \param  request       Flick request structure
 * \param  ds_id         Dataspace id
 * \retval _ev           Flick exception structure, unused
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_EINVAL invalid dataspace id
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_memphys_server_show_ds(sm_request_t * request, 
			       l4_uint32_t ds_id, 
			       sm_exc_t * _ev)
{
  dmphys_dataspace_t * ds;
  
  /* get dataspace descriptor */
  ds = dmphys_ds_get(ds_id);
  if (ds == NULL)
    {
      ERROR("DMphys: invalid dataspace %u, caller %x.%x!",ds_id,
	    request->client_tid.id.task,request->client_tid.id.lthread);
      return -L4_EINVAL;
    }

  /* show dataspace info */
  dmphys_ds_show(ds);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Dump dataspace information to dataspace
 * 
 * \param  request       Flick request structure
 * \param  owner         Dataspace owner, if set to L4_INVALID_ID list
 *                       all dataspaces
 * \param  flags         Flags:
 *                       - #L4DM_SAME_TASK  dump dataspaces owned by task
 * \retval dump_ds       Dataspace id
 * \retval _ev           Flick exception structure, unused
 *	
 * \return 0 on success, error code otherwise:
 *         - \c -L4_ENOHANDLE  could not create dataspace descriptor
 *         - \c -L4_ENOMEM     out of memory
 */
/*****************************************************************************/ 
l4_int32_t 
if_l4dm_memphys_server_dump(sm_request_t * request, 
			    const if_l4dm_threadid_t * owner, 
			    l4_uint32_t flags, 
			    if_l4dm_dataspace_t * dump_ds, 
			    sm_exc_t * _ev)
{
  int num,ret,printed;
  l4_size_t size;
  dmphys_dataspace_t * ds;
  l4_threadid_t o = *(l4_threadid_t *)owner;
  dump_arg_t arg;

  /* calculate dataspace size */
  num = dmphys_ds_count(o,flags);
  size = 80 * (num + 1);

  /* allocate dataspace */
  ret = dmphys_open(request->client_tid,dmphys_get_default_pool(),
		    L4DM_MEMPHYS_ANY_ADDR,size,0,L4DM_CONTIGUOUS,
		    "l4dm_ds_dump data",(l4dm_dataspace_t *)dump_ds);
  if (ret < 0)
    {
      ERROR("DMphys: create dump dataspace failed: %d!",ret);
      return ret;
    }
  ds = dmphys_ds_get(dump_ds->id);

  arg.sum = 0;
  arg.size = size;
  arg.str_buf = (char *)AREA_MAP_ADDR(dmphys_ds_get_pages(ds));

  /* iterate dataspace list */
  dmphys_ds_iterate(__ds_dump_iterator,&arg,o,flags);

  if (arg.sum > 0)
    printed = 
      snprintf(arg.str_buf,arg.size-1,
	       "===========================================================\n"
	       "total size %08x (%dkB)\n",arg.sum,arg.sum/1024);
  else
    printed = snprintf(arg.str_buf,arg.size-1," no suitable dataspace found\n");
  
  arg.size    -= printed;
  arg.str_buf += printed;
  *arg.str_buf = '\0';

  /* free unused pages */
  if (arg.size > 0)
    dmphys_resize(ds,size - arg.size + 1);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  List dataspaces
 * 
 * \param  request       Flick request structure
 * \param  owner         Dataspace owner, if set to L4_INVALID_ID list
 *                       all dataspaces
 * \param  flags         Flags:
 *                       - #L4DM_SAME_TASK  list dataspaces owned by task
 * \retval _ev           Flick exception structure, unused
 */
/*****************************************************************************/ 
void 
if_l4dm_memphys_server_list(sm_request_t * request, 
			    const if_l4dm_threadid_t * owner, 
			    l4_uint32_t flags, 
			    sm_exc_t * _ev)
{
  l4_size_t sum = 0;
  l4_threadid_t o = *(l4_threadid_t *)owner;

  if (l4_is_invalid_id(o))
    Msg("DMphys dataspace list:\n");
  else
    Msg("DMphys dataspace list for client %x.%x:\n",o.id.task,o.id.lthread);

  /* iterate dataspace list */
  dmphys_ds_iterate(__ds_list_iterator,&sum,o,flags);

  if (sum > 0)
    Msg("===========================================================\n"
	"total size %08x (%dkB)\n", sum, sum/1024);
  else
    Msg("no suitable dataspace found\n");

  /* done */
}
