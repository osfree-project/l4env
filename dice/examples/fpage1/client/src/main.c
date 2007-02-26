#include "test-client.h"
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/util/util.h>

#define MAP_ADDRESS 0x99c000

static
void test_client(CORBA_Object _server)
{
  l4_snd_fpage_t page;
  unsigned long *ptr, ret, offset=0x20;
  CORBA_Environment _env = dice_default_environment;
  // set receive position for flexpage
  _env.rcv_fpage = l4_fpage(MAP_ADDRESS, 12/*1 page*/, L4_FPAGE_RW, L4_FPAGE_GRANT);
  // establich mapping
  ret = test_test_map_call(_server, offset, &page, &_env);
  if (ret)
    {
      LOG("map failed");
      if (ret == 1)
	LOG("because offset is to big");
      else if (ret == 2)
	LOG("because server couldn't get memory");
      else if (ret == 3)
	LOG("because server couldn't attach ds");
      return;
    }
  // check data at client
  ptr = (unsigned long*)(MAP_ADDRESS + offset);
  LOG("addr = 0x%08x", ptr);
  LOG("data = %d", *ptr);
  // check data at server
  ret = test_test_check_call(_server, &_env);
}

int main(int argc, char* argv[])
{ 
  l4_threadid_t _server;
  LOG_init("fpageC");
  // find server
  names_waitfor_name("fpageS", &_server, 120);
  // call server
  test_client(&_server); 

  l4_sleep(2000);
  enter_kdebug("*#^fpage1 stopped");
  return 0;
}

