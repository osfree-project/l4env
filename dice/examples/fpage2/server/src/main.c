#include "test-server.h"
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/dm_mem/dm_mem.h>

#define WORK_AROUND_FIASCO

#ifdef WORK_AROUND_FIASCO
unsigned long *glob_ptr;
#endif

CORBA_long test_test_map_component(CORBA_Object *_dice_corba_obj,
    l4_snd_fpage_t page,
    CORBA_unsigned_long offset,
    CORBA_Environment *_dice_corba_env)
{
  unsigned long *ptr;

#ifdef WORK_AROUND_FIASCO
  ptr = (unsigned long*)((unsigned long)glob_ptr + offset);
#else
  /* get page for variable data */
  ptr = (unsigned long*)(page.snd_base + offset);
#endif
  
  LOG("address = 0x%x", ptr);
  LOG("page = 0x%x", page.snd_base);
  LOG("data at serv before = %d\n", *ptr);
  *ptr = 12345;
  LOG("data at serv after = %d", *ptr);

  /* done */    
  return 0;
}

int main(int argc, char* argv[])
{
  void *ptr;
  CORBA_Environment _env = dice_default_environment; // timout:never, rcv_fpage:WHOLE_AS
  
  LOG_init("fpageS");
  /* get a receive window (need only 1 page)
   */
  ptr = l4dm_mem_allocate(0x1000,0);
  if (!ptr)
    enter_kdebug("dm_mem_allocate failed");
#ifdef WORK_AROUND_FIASCO
  glob_ptr = ptr;
#endif
  _env.rcv_fpage = l4_fpage((unsigned long)ptr, 12, 1, 1);
  LOG("receive at 0x%x\n", ptr);
  
  // register with names
  names_register("fpageS");
  // turn on logging
  enter_kdebug("*#I*IR+");
  // start loop
  test_test_server_loop(&_env);
  // unregister with names
  names_unregister("fpageS");
  return 0;
}

