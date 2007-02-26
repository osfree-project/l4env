/*!
 * \file   names/lib/src/names_dump.c
 * \brief  Implementation of names_dump()
 *
 * \date   11/03/2003
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/names/libnames.h>

#include "names-client.h"
#include "__libnames.h"

/*!\brief Dumps the entire mapping of id <-> names.
 * \ingroup clientapi
 *
 * \retval 0	Error. (This can be only IPC error.)
 * \retval !=0	Success.
 */
int
names_dump(void)
{
  CORBA_Environment env = dice_default_environment;
  l4_threadid_t *ns_id = names_get_ns_id();

  if (!ns_id)
    return 0;

  names_dump_call(ns_id, &env);

  return 1;
}
