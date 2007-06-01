/*!
 * \file   names/server/src/predefined.c
 * \brief  handling of predefined names: rmgr, sigma0
 *
 * \date   01/27/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <string.h>
#include <stdio.h>
#include <l4/rmgr/librmgr.h>
#include <l4/rmgr/proto.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>

#include <names.h>

/*!\brief pre-register threads
 *
 * by convention, we know:
 * 0.0	kernel idle thread
 * 2.0	Sigma0
 * X.x	rmgr, pager thread. Task dynamic, thread from librmgr.h
 * X.y	rmgr, service thread.
 */
int preregister(void)
{
    char buffer[NAMES_MAX_NAME_LEN];
    l4_threadid_t me = l4_myself();
    l4_threadid_t id;
    int i;

    id = L4_NIL_ID;
    if (!server_names_register(&me, "kernel idler", &id, 0))
      return 0;

    id.id.task = 2;
    if (!server_names_register(&me, "sigma0", &id, 0))
      return 0;


    id = rmgr_id;
    id.id.lthread = RMGR_LTHREAD_PAGER;
    if (!server_names_register(&me, "rmgr.pager", &id, 0))
      return 0;

    id.id.lthread = RMGR_LTHREAD_SUPER;
    if (!server_names_register(&me, "rmgr.service", &id, 0))
      return 0;

    for (i=0; i < RMGR_IRQ_MAX; i++) {
	id.id.lthread = i + RMGR_IRQ_LTHREAD;
	sprintf(buffer, "rmgr.irq%02X", i);
	if (!server_names_register(&me, buffer, &id, 0))
	  return 0;
    }

    id = l4_myself();
    if (!server_names_register(&me, "names", &id, 0))
      return 0;

    return 1;
}
