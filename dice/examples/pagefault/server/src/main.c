#include "test-server.h"
#include "test-client.h"
#include <l4/log/l4log.h>
#include <l4/rmgr/librmgr.h>
#include <l4/thread/thread.h>
#include <l4/util/util.h> // needed for l4_sleep
#include <l4/util/reboot.h> // needed for reboot
#include <l4/sys/syscalls.h> // needed for l4_myself

#define STACK_PAGES 6
#define STACK_PAGE_SIZE (4*2024)
#define STACK_SIZE_BYTES (STACK_PAGES*STACK_PAGE_SIZE)

static char stack[STACK_SIZE_BYTES];
static l4_umword_t stack_addr;

char LOG_tag[9] = "fpage";

static
void test_server(void)
{
  int i;
  long d;
  l4_umword_t addr;
  // poke around stack, which should produce page faults
  for (i=0; i<STACK_PAGES; i++)
    {
      addr = stack_addr+i*STACK_PAGE_SIZE+STACK_PAGE_SIZE/2;
      d = *(long*)addr;
      LOG("value at %x is %ld", addr, d);
    }
  LOG("client finished");
  // sleep forever
  l4_sleep(-1);
}

static
void test_client(CORBA_Object server)
{
  CORBA_Environment env;
  long status;

  handler_check_status_call(server, &status, &env);
  LOG("returned %ld", status);
}

int main(int argc, char**argv)
{
  l4thread_t srv;
  l4_taskid_t clnt;
  l4_threadid_t l4srv;
  rmgr_init();
  // start pager thread, which runs handler-loop
  srv = l4thread_create(handler_server_loop, NULL, L4THREAD_CREATE_ASYNC);
  l4srv = l4thread_l4_id(srv);
  // set stack address
  stack_addr = (l4_umword_t)&stack;
  LOG("Stack at 0x%x[%x]",stack_addr,STACK_SIZE_BYTES);
  // zero out stack
  memset((void*)&stack, 0, STACK_SIZE_BYTES);
  // create "user" task
  clnt = l4_myself();
  clnt.id.lthread = 0;
  while (rmgr_get_task(++(clnt.id.task)) == -1) ;
  LOG("got id %x",clnt.id.task);
  clnt = l4_task_new(clnt, 0, (l4_umword_t)&stack, (l4_umword_t)test_server, l4srv);
  LOG("created task is %x",clnt.id.task);
  // call client test function
  test_client(&l4srv);
  
  l4_sleep(2000);
  l4util_reboot();
  // finished
  return 0;
}
