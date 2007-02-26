/*!
 * \file   names/lib/src/names_query_id.c
 * \brief  Implementation of names_query_id()
 *
 * \date   05/27/2003
 * \author Uwe Dannowski <Uwe.Dannowski@ira.uka.de>
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <names.h>
#include <l4/sys/syscalls.h>
#include <string.h>

/*!\brief Get the name registered for a thread ID
 * \ingroup clientapi
 *
 * \param  id		The thread ID the name should be returned for.
 * \param  name		ptr to a preallocated area of memory
 * \param  length	size of the area at name
 *
 * \retval 0		Error. The thread ID was not being registered before.
 * \retval !=0		Success.
 *
 *  The name service is queried for the thread_id id. If there is a
 *  name registered for id, the associated name is copied to the
 *  buffer referenced by name.
 */
int
names_query_id(const l4_threadid_t id, char* name, const int length)
{
  message_t message;
  char	    buffer[NAMES_MAX_NAME_LEN+1];
  int	    ret;

  names_init_message(&message, buffer);
  message.cmd = NAMES_QUERY_ID;
  message.id = id;

  ret = names_send_message(&message);
  if (ret)
    strncpy(name, buffer, NAMES_MAX_NAME_LEN < length ?
                          NAMES_MAX_NAME_LEN : length);
  return ret;
};
