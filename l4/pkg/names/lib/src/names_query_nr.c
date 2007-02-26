/*!
 * \file   names/lib/src/names_query_nr.c
 * \brief  Implementation of names_query_nr()
 *
 * \date   05/27/2003
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

/*!\brief Query the entry of the given number.
 * \ingroup clientapi
 *
 * \param  nr		The nr the name and thread ID should be returned for.
 * \param  name		Pointer to a preallocated area of memory. May be
 *                      NULL.
 * \param  length	Size of the area at name.
 * \param  id		Thread ID of the entry number nr. May be NULL.
 *
 * \retval 0		Error. The entry with the given number is not valid.
 * \retval !=0		Success.
 *
 * The name service is queried for the given number. Upon success, both name
 * and id are filled in. To get all entries registered at the server, you
 * have to iterate from 0 to ::NAMES_MAX_ENTRIES and take all successful
 * answers into account.
 */
int
names_query_nr(const int nr, char* name, const int length, l4_threadid_t *id)
{
  CORBA_Environment env = dice_default_environment;
  char buffer[NAMES_MAX_NAME_LEN+1];
  char *_buffer = buffer;
  int ret;
  l4_threadid_t *ns_id = names_get_ns_id();
  l4_threadid_t ret_id;

  if (!ns_id)
    return 0;

  ret = names_query_nr_call(ns_id, nr, &_buffer, &ret_id, &env);

  if (ret)
    {
      if (name)
	strncpy(name, buffer, NAMES_MAX_NAME_LEN < length ?
	                      NAMES_MAX_NAME_LEN : length);
      if (id)
	*id = ret_id;
    }
  return ret;
}
