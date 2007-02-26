/*!
 * \file   names/lib/src/names_query_name.c
 * \brief  Implementation of names_query_name()
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

/*!\brief Get the thread ID registered for a given name
 * \ingroup clientapi
 *
 * \param  name		0-terminated name the ID should be returned for.
 * \param  id		thread ID will be stored here.
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
  message_t message;
  char	    buffer[NAMES_MAX_NAME_LEN+1];
  int	    ret;

  names_init_message(&message, buffer);
  message.cmd = NAMES_QUERY_NAME;
  strncpy(buffer, name, NAMES_MAX_NAME_LEN);
  
  ret = names_send_message(&message);
  if (ret)
    *id = message.id;
  return ret;
};
