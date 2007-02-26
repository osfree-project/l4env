/*!
 * \file   names/lib/src/names_unregister_task.c
 * \brief  Implementation of names_unregister_task()
 *
 * \date   05/27/2003
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
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

/*!\brief Unregister all names registered for a given task.
 * \ingroup clientapi
 *
 * \param  tid		contains the task ID to unregister
 *
 * \retval 0		Error. No entry found matching the given task.
 * \retval !=0		Success. At least one entry matching the given task
 *			found.
 *
 * All names/ID pairs with the task-ID part of the thread ID matching that of
 * tid are unregistered at the server.
 */
int
names_unregister_task(l4_threadid_t tid)
{
  CORBA_Environment env = dice_default_environment;
  l4_threadid_t *ns_id = names_get_ns_id();

  if (!ns_id)
    return 0;

  return names_unregister_task_call(ns_id, &tid, &env);
}
