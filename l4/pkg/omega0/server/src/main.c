#include <omega0_proto.h>
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
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
#include "pic.h"
#include "config.h"
#include "events.h"


unsigned MANAGEMENT_THREAD = 0;


int main(int argc, const char**argv)
{
  int error;

  if ((error = parse_cmdline(&argc, &argv,
	       'o', "nosfn", "don't use special fully nested mode",
	       PARSE_CMD_SWITCH, 0, &use_special_fully_nested_mode,
      	       'e', "events", "enable exit handling via events",
	       PARSE_CMD_SWITCH, 1, &use_events,
	       0)))
    return 1;

  rmgr_init();
  LOG_printf("Using %s fully nested PIC mode\n",
	     use_special_fully_nested_mode ? "special" : "(normal)");
  
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
