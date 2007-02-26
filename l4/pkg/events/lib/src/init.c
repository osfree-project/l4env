/*!
 * \file   events/lib/src/init.c
 *
 * \brief  IPC stub initialization
 *
 * \date   09/14/2003
 * \author Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <stdio.h>

#include "l4/events/events.h"
#include <l4/names/libnames.h>
#include <l4/sys/kdebug.h>

#include "lib.h"
#include "message.h"


/*!\brief initialize the library
 * \ingroup internal
 *
 * This function uses names to the get thread ID of events server.
 */
int
l4events_init(void)
{
  if (!l4_is_invalid_id(l4events_server))
    return 1;

  if (!names_waitfor_name("event_server", &l4events_server, 5000))
    printf("Event server not found!\n");

  return l4_is_invalid_id(l4events_server) ? 0 : 1;
}
