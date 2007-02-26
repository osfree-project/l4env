/*!
 * \file   names/lib/src/names_query_name.c
 * \brief  Implementation of names_query_name()
 *
 * \date   05/27/2003
 * \author Uwe Dannowski <Uwe.Dannowski@ira.uka.de>
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/names/libnames.h>

#include "names-client.h"
#include "__libnames.h"

/*!\brief Get the thread ID registered for a given name
 * \ingroup clientapi
 *
 * \param  name		0-terminated name the ID should be returned for.
 * \param  id		Thread ID of the name. May be NULL.
 *
 * \retval 0		Error. The name was not being registered before.
 * \retval !=0		Success.
 *
 * The name service is queried for the string name. If found, the
 * associated thread_id is written to the buffer referenced by id.
 */
int
names_query_name(const char* name, l4_threadid_t* id)
{
  CORBA_Environment env = dice_default_environment;
  l4_threadid_t *ns_id = names_get_ns_id();
  l4_threadid_t ret_id;
  int ret;

  if (!ns_id)
    return 0;

  ret = names_query_name_call(ns_id, name, &ret_id, &env);
  if (ret && id)
    *id = ret_id;
  return ret;
}
