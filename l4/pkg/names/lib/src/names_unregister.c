/*!
 * \file   names/lib/src/names_unregister.c
 * \brief  Implementation of names_unregister()
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
#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>

#include "names-client.h"
#include "__libnames.h"

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
  CORBA_Environment env = dice_default_environment;
  l4_threadid_t *ns_id = names_get_ns_id();
  l4_threadid_t my_id = l4_myself();

  if (!ns_id)
    return 0;

  return names_unregister_thread_call(ns_id, name, &my_id, &env);
}
