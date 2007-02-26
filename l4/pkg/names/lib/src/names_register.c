/*!
 * \file   names/lib/src/names_register.c
 * \brief  Implementation of names_register()
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


/*!\brief Register the current thread with the given name
 * \ingroup clientapi
 *
 * \param  name		0-terminated name to register as.
 *
 * \retval 0		Error. Name is already registerd.
 * \retval !=0		Success.
 *
 * The string name is registered with the caller's own thread_id.  
 *
 * \note   The name is not necessarily 0-terminated when being sent to the
 *         server.
 */
int
names_register(const char* name)
{
  message_t message;
  char	    buffer[NAMES_MAX_NAME_LEN];

  names_init_message(&message, buffer);

  message.cmd = NAMES_REGISTER;
  message.id = l4_myself();
  strncpy(buffer, name, NAMES_MAX_NAME_LEN);

  return names_send_message(&message);
};
