#include "test-server.h"
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/dm_mem/dm_mem.h>

unsigned long *global_ptr = 0;

CORBA_long test_test_map_component(CORBA_Object _dice_corba_obj,
    CORBA_unsigned_long offset,
    l4_snd_fpage_t *page,
    CORBA_Environment *_dice_corba_env)
{
  unsigned long *ptr;
  l4dm_dataspace_t ds;
  void * addr = 0;

  LOG("received offset 0x%x", offset);

  /* if offset is out of our reach, return error */
  if (offset >= 8192)
    return 1;

  /* get dataspace for this process */
  // allocate memory, see DMphys reference manual
  if (l4dm_mem_open(L4DM_DEFAULT_DSM,8192,0,0,"dice test",&ds))
    {
      LOG("l4dm_mem_open failed");
      return 2;
    }

  // attach dataspace to my address space
  if (l4rm_attach(&ds,8192,0,L4DM_RW|L4RM_MAP,&addr))
    {
      LOG("l4rm_attach failed");
      return 3;
    }
  LOG("attached DS at addr 0x%08x", addr);
  ptr = (unsigned long*)(addr + offset);
  
  // do something with the meomry, it is attached to address addr
  global_ptr = ptr;
  LOG("address = 0x%08x", ptr);
  *ptr = 12345;
  LOG("data = %d", *ptr);
  
  /* is the address we touched page at */
  page->snd_base = (l4_mword_t)addr;
  /* calc fpage descriptor for page */
  page->fpage = l4_fpage((l4_mword_t)addr,12,L4_FPAGE_RW/*_RO*/,L4_FPAGE_MAP);
    
  // detach dataspace 
  //l4rm_detach(addr);
  // close dataspace
  //l4dm_close(&ds);
  
  /* done */    
  return 0;
}

CORBA_long test_test_check_component(CORBA_Object _dice_corba_obj,
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
