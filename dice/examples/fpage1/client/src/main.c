#include "test-client.h"
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/util/util.h>

#define MAP_ADDRESS 0x99b000

static
void test_client(CORBA_Object _server)
{
  l4_snd_fpage_t page;
  unsigned long *ptr, ret;
  CORBA_Environment _env = dice_default_environment;

  // establich mapping
  ret = test_test_map_call(&_server, MAP_ADDRESS, 0x20, &page, &_env);
  // check data at client
  ptr = (unsigned long*)(MAP_ADDRESS + 0x20);
  LOG("data = %d", *ptr);
  // check data at server
  ret = test_test_check_call(&_server, &_env);
}

int main(int argc, char* argv[])
{ 
  CORBA_Object _server;
  LOG_init("fpageC");
  // find server
  names_waitfor_name("fpageS", &_server, 120);
  // call server
  test_client(_server); 

  return 0;
}

