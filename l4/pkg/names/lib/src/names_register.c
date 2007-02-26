/*!
 * \file   names/lib/src/names_register.c
 * \brief  Implementation of names_register()
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

#include "__libnames.h"
#include "names-client.h"

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
  CORBA_Environment env = dice_default_environment;
  l4_threadid_t *ns_id = names_get_ns_id();

  if (!ns_id)
    return 0;

  return names_register_call(ns_id, name, &env);
}
