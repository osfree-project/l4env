#include <omega0_proto.h>
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/rmgr/librmgr.h>
#include <l4/oskit/support.h>
#include <string.h>
#include <stdlib.h>

#include "globals.h"
#include "irq_threads.h"
#include "server.h"
#include "config.h"

/* krishna: 16 kb of data for dynamic allocation + dynamic stack allocation */
#define MEM_SIZE (1024*16 + IRQ_NUMS*STACKSIZE)
static char mem_array[MEM_SIZE] __attribute__ ((aligned(4096)));

static int mem_init(void);
static int parse_args(int, char*[]);

/* initalized list-based memory manager from oskit. We use <memsize> bytes
   within mem_array. */
static int mem_init(void){
  void *addr;
  
  addr = &mem_array;

  init_OSKit_malloc_from_memory((l4_umword_t)addr, MEM_SIZE);
  
  return 0;
}

static int parse_args(int argc, char*argv[]){
  // currently, we parse no arguments
  return 0;
}

int main(int argc, char**argv){
  rmgr_init();
  LOG_init("omega0");
  
  parse_args(argc, argv);
  mem_init();
  
  LOGl("memory initialized");
  
  attach_irqs();
  
  LOGl("attached to irqs");
  
  while(names_register(OMEAG0_SERVER_NAME)==0){
    LOGl("error registering at nameserver");
    enter_kdebug("!");
  }
  
  LOGl("registered at nameserver");
  
  server();
  
  while(1){
    LOGL("server() returned!");
    enter_kdebug("halted");
  }

  return 0;
}
