/*!
 * \file   names/lib/src/libnames.c
 * \brief  Client side of names.
 *
 * \author Uwe Dannowski <Uwe.Dannowski@ira.uka.de>
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>

#include <l4/rmgr/librmgr.h>
#include <l4/util/util.h>

#include <l4/names/libnames.h>

#include "__libnames.h"
#include "names-client.h"

static l4_threadid_t ns_id;
static int initialized;

/*!\brief Initialize the lib
 * \ingroup internal
 *
 * This function uses rmgr to the get thread ID of names.
 * This function needs not to be called explicitely by a library user.
 */
static int
names_init(void)
{
  if (!rmgr_init())
    return 0;

  if (rmgr_get_task_id("names", &ns_id))
    return 0;

  if (l4_is_invalid_id(ns_id))
    return 0;

  initialized = 1;
  return 1;
};

/*!\brief Return ID of name server.
 * \ingroup internal
 */
l4_threadid_t *
names_get_ns_id(void)
{
  if (!initialized && !names_init())
      return NULL;

  return &ns_id;
}
