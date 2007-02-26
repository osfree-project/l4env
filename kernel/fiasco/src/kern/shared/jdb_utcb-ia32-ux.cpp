INTERFACE[{ia32,ux}-v2-lipc]:

#include "jdb_module.h"
#include "static_init.h"

/**
 * @brief Jdb-Utcb module
 *
 * This module shows the user tcbs of a space
 */
 
class Utcb;

class Utcb_m
  : public Jdb_module
{
  static char buff[10];
};


static Utcb_m Utcb_m INIT_PRIORITY(JDB_MODULE_INIT_PRIO);


IMPLEMENTATION[{ia32,ux}-v2-lipc]:

#include "l4_types.h"
#include "config.h"
#include "assert.h"
#include <cstdio>
#include "space_index.h"
#include "space.h"
#include "thread.h"

char Utcb_m::buff[10];

PUBLIC
Utcb_m::Utcb_m()
  : Jdb_module("INFO")
{ 
}

PUBLIC virtual
Jdb_module::Action_code
Utcb_m::action( int cmd, void *&args, char const *&, int &)
{
  if (cmd != 0) 
    return NOTHING;
  if ((char*)args != buff) 
    return NOTHING;

  unsigned int task  = *((unsigned int*) buff);
  unsigned int thread= *(((unsigned int*) buff)+1);

  if ((task == Config::kernel_taskno) || (task > 2048)) 
    {
      printf("\ninvalid space\n");
      return NOTHING;
    }
  
  if (thread > 127) 
    {
      printf("\ninvalid thread\n");
      return NOTHING;
    }

  Space *s = Space_index(task).lookup();
  if (!s) 
    {
      printf("\ninvalid space\n");
      return NOTHING;
    }

  L4_uid id(task, thread);
  Thread *t = Thread::lookup(id);
  if(!t)
    {
      printf("\ninvalid thread\n");
      return NOTHING;
    }
  
  printf("\n"
         "thread:   %d.%02x\n\n"
         "global status\n"
         "current utcb:  "L4_PTR_FMT"\n"
         "TCB state\n"
         "utcbaddr:  "L4_PTR_FMT"\n\n",
	 task, thread,
	 *global_utcb_ptr,
         (Address)(Mem_layout::V2_utcb_addr + thread * sizeof(Utcb)));
  t->utcb()->print();
  return NOTHING;
}

PUBLIC 
int const
Utcb_m::num_cmds() const
{ 
  return 1;
}



PUBLIC
Jdb_module::Cmd
const *const Utcb_m::cmds() const
{
  static Cmd cs[] =
    {
      { 0, "z", "z", " space: %u thread: %i",
	"Z\tShow user tcb", Utcb_m::buff }
    };
  return cs;
}

