#include <omega0_proto.h>
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/rmgr/librmgr.h>
#ifdef USE_OSKIT
#include <l4/oskit/support.h>
#endif
#include <l4/util/parse_cmd.h>
#include <string.h>
#include <stdlib.h>

#include "globals.h"
#include "irq_threads.h"
#include "server.h"
#include "pic.h"
#include "config.h"

#ifdef USE_OSKIT
/* krishna: 16 kb of data for dynamic allocation + dynamic stack allocation */
#define MEM_SIZE (1024*16 + IRQ_NUMS*STACKSIZE)
static char mem_array[MEM_SIZE] __attribute__ ((aligned(4096)));
#endif

/* initalized list-based memory manager from oskit. We use <memsize> bytes
   within mem_array. */
static int mem_init(void){
#ifdef USE_OSKIT
    void *addr;
  
    addr = &mem_array;
    init_OSKit_malloc_from_memory((l4_umword_t)addr, MEM_SIZE);
#endif
    return 0;
}

int main(int argc, const char**argv){
    int error;

    if ((error = parse_cmdline(&argc, &argv,
		 'o', "nosfn", "don't use special fully nested mode",
		 PARSE_CMD_SWITCH, 0, &use_special_fully_nested_mode,
		 0)))
      return 1;

    rmgr_init();
    LOG("Using %s fully nested PIC mode",
	 use_special_fully_nested_mode ? "special" : "(normal)");
  
    mem_init();
    LOGdl(OMEGA0_DEBUG_STARTUP,"memory initialized");
  
    attach_irqs();
    LOGdl(OMEGA0_DEBUG_STARTUP,"attached to irqs");
  
    if(names_register(OMEAG0_SERVER_NAME)==0){
	LOGl("error registering at nameserver");
	return 1;
    }
    LOGdl(OMEGA0_DEBUG_STARTUP,"registered at nameserver");
  
    server();
    return 0;
}
