/*!
 * \file   names/lib/src/names_query_nr.c
 * \brief  Implementation of names_query_nr()
 *
 * \date   05/27/2003
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

/*!\brief Query the entry of the given number.
 * \ingroup clientapi
 *
 * \param  nr		The nr the name and thread ID should be returned for.
 * \param  name		ptr to a preallocated area of memory
 * \param  length	size of the area at name
 * \param  id		thread ID will be stored here.
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
  message_t message;
  char	    buffer[NAMES_MAX_NAME_LEN+1];
  int	    ret;

  names_init_message(&message, buffer);
  message.cmd = NAMES_QUERY_NR;
  message.id.lh.low = nr;

  ret = names_send_message(&message);
  if (ret)
    strncpy(name, buffer, NAMES_MAX_NAME_LEN < length ?
                          NAMES_MAX_NAME_LEN : length);
    *id = message.id;
  return ret;
};
