#include "test-client.h"
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/util/util.h>
#include <l4/util/reboot.h>

unsigned char array[8096];

static
void test_client(CORBA_Object _server)
{
  CORBA_long ret;
  l4_snd_fpage_t page;
  unsigned long offset, *ptr;
  unsigned char *base; 
  CORBA_Environment _env = dice_default_environment;
  /* calculate start address 
   * should be 4KB aligned address in array
   */
  base = (unsigned char*)l4_round_page(array); // align to 4KB

  /* get page for variable data */
  offset = 0x200;
  ptr = (unsigned long*)((unsigned long)base + offset);
  LOG("ptr = 0x%p", ptr);
  LOG("data = %ld", *ptr);
  *ptr = 6789;
  LOG("data = %ld", *ptr);

  /* calc base addsess of page */
  /* is the address we touched page at */
  page.snd_base = (l4_umword_t)base;
  /* calc fpage descriptor for page */
  page.fpage = l4_fpage((l4_umword_t)base, 12, L4_FPAGE_RW/*_RO*/, L4_FPAGE_MAP);
  LOG("send fpage: 0x%08x", page.fpage.raw);

  /* get fpage descriptor */
  // call client stub
  LOG("page calc done, send page");
  ret = test_test_map_call(_server, page, offset, &_env);
  LOG("server returned %ld", ret);

  if (_env.major != CORBA_NO_EXCEPTION)
    {
      LOG("Exception %d (%d) ipc:%x", _env.major, _env.repos_id, _env._p.ipc_error);
    }

  LOG("data after call=%ld", *ptr);
}

char LOG_tag[9] = "fpageC";

int main(int argc, char* argv[])
{ 
  // needed for client
  l4_threadid_t _server;
  // find server
  names_waitfor_name("fpageS", &_server, 120);
  LOG("found server, start test");
  // call server
  test_client(&_server);

  l4_sleep(2000);
  l4util_reboot();
  return 0;
}

