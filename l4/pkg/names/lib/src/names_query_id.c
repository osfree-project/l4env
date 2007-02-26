/*!
 * \file   names/lib/src/names_query_id.c
 * \brief  Implementation of names_query_id()
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

/*!\brief Get the name registered for a thread ID
 * \ingroup clientapi
 *
 * \param  id		The thread ID the name should be returned for.
 * \param  name		Pointer to a preallocated area of memory. May be
 *                      NULL.
 * \param  length	Size of the area at name
 *
 * \retval 0		Error. The thread ID was not being registered before.
 * \retval !=0		Success.
 *
 *  The name service is queried for the thread_id id. If there is a
 *  name registered for id, the associated name is copied to the
 *  buffer referenced by name.
 */
int
names_query_id(const l4_threadid_t id, char *name, const int length)
{
  CORBA_Environment env = dice_default_environment;
  char buffer[NAMES_MAX_NAME_LEN+1];
  char *_buffer = buffer;
  int ret;
  l4_threadid_t *ns_id = names_get_ns_id();

  if (!ns_id)
    return 0;

  ret = names_query_id_call(ns_id, &id, &_buffer, &env);

  if (ret && name)
    strncpy(name, buffer, NAMES_MAX_NAME_LEN < length ?
                          NAMES_MAX_NAME_LEN : length);
  return ret;
}
