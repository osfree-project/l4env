/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/lib/src/clients.c
 * \brief  Region mapper client handling
 *
 * \date   02/19/2002
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
#include <l4/util/macros.h>

/* private includes */
#include <l4/l4rm/l4rm.h>
#include "__region.h"
#include "__debug.h"

/*****************************************************************************/
/**
 * \brief  Add new region mapper client
 * 
 * \param  client        Client thread id
 *	
 * \return 0 on success (added client), error code otherwise:
 *         - \c -L4_EIPC  calling region mapper thread failed
 */
/*****************************************************************************/ 
int
l4rm_add_client(l4_threadid_t client)
{
  /* add client */
  return l4rm_tree_add_client(client,0);
}

/*****************************************************************************/
/**
 * \brief  Add new client without calling region mapper thread (SETUP ONLY!)
 * 
 * \param  client        Client thread id
 *	
 * \return 0 on success (added client)
 */
/*****************************************************************************/ 
int
l4rm_direct_add_client(l4_threadid_t client)
{
  /* add client */
  return l4rm_tree_add_client(client,MODIFY_DIRECT);
}

/*****************************************************************************/
/**
 * \brief  Remove region mapper client
 * 
 * \param  client        Client thread id
 *	
 * \return 0 on success (remove client), error code otherwise:
 *         - \c -L4_EIPC  calling region mapper thread failed
 */
/*****************************************************************************/ 
int
l4rm_remove_client(l4_threadid_t client)
{
  /* remove client */
  return l4rm_tree_remove_client(client);
}
