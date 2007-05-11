/**
 * \file   omega0/server/src/main.c
 * \brief  Main routine
 *
 * \date   2007-04-27
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <omega0_proto.h>
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/sigma0/kip.h>
#include <l4/util/util.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/rmgr/librmgr.h>
#include <l4/util/parse_cmd.h>
#include <string.h>
#include <stdlib.h>

#include "globals.h"
#include "irq_threads.h"
#include "server.h"
#include "config.h"
#include "events.h"


unsigned MANAGEMENT_THREAD = 0;


int main(int argc, const char**argv)
{
  int error;

  if ((error = parse_cmdline(&argc, &argv,
      	       'e', "events", "enable exit handling via events",
	       PARSE_CMD_SWITCH, 1, &use_events,
	       0)))
    return 1;

  rmgr_init();

  unsigned abi_version;
  if ((abi_version = l4sigma0_kip_kernel_abi_version()) < 9)
    {
      LOG_Error("Fiasco kernel too old (current ABI %d - need >=9)", abi_version);
      return 2;
    }

  attach_irqs();
  LOGdl(OMEGA0_DEBUG_STARTUP,"attached to irqs");

  if (use_events)
    {
      init_events();
      LOGdl(OMEGA0_DEBUG_STARTUP,"started events thread");
    }
  
  if(names_register(OMEAG0_SERVER_NAME)==0)
    {
      LOGl("error registering at nameserver");
      return 1;
    }
  LOGdl(OMEGA0_DEBUG_STARTUP,"registered at nameserver");
  
  server();
  return 0;
}
