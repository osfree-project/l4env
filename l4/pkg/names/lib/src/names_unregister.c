/*!
 * \file   names/lib/src/names_unregister.c
 * \brief  Implementation of names_unregister()
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

/*!\brief Unregister a given name.
 * \ingroup clientapi
 *
 * \param  name		0-terminated name to unregister.
 *
 * \retval 0		Error. Name is not registerd.
 * \retval !=0		Success.
 *
 * The string name is unregistered.
 */
int
names_unregister(const char* name)
{
  message_t message;
  char	    buffer[NAMES_MAX_NAME_LEN+1];

  names_init_message(&message, buffer);
  
  message.cmd = NAMES_UNREGISTER;
  message.id = l4_myself();
  strncpy(buffer, name, NAMES_MAX_NAME_LEN);

  return names_send_message(&message);
};
