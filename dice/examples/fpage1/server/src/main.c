#include "test-server.h"
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>

unsigned long *global_ptr = 0;

CORBA_long test_test_map_component(CORBA_Object *_dice_corba_obj,
    CORBA_unsigned_long address,
    CORBA_unsigned_long offset,
    l4_snd_fpage_t *page,
    CORBA_Environment *_dice_corba_env)
{
  unsigned long *ptr;
  
  /* get page for variable data */
  ptr = (unsigned long*)(address + offset);
  global_ptr = ptr;
  LOG("address = 0x%x", ptr);
  *ptr = 12345;
  LOG("data = %d", *ptr);
  
  /* calc base addsess of page */
  /* is the address we touched page at */
  page->snd_base = address;
  /* calc fpage descriptor for page */
  page->fpage = l4_fpage(address,12,L4_FPAGE_RW/*_RO*/,L4_FPAGE_MAP);
  
  /* done */    
  return 0;
}

CORBA_long test_test_check_component(CORBA_Object *_dice_corba_obj,
    CORBA_Environment *_dice_corba_env)
{
  if (global_ptr)
    {
      LOG("data = %d", *global_ptr);
    }
  else
    {
      LOG("global_ptr not set");
    }
  return 1;
}

int main(int argc, char* argv[])
{
  LOG_init("fpageS");
  // register with names
  names_register("fpageS");
  // start loop
  test_test_server_loop(NULL);
  // unregister with names
  names_unregister("fpageS");
  return 0;
}
