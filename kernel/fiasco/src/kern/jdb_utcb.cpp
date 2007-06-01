/**
 * @brief Jdb-Utcb module
 *
 * This module shows the user tcbs of a space
 */
 
IMPLEMENTATION:

#include <cassert>
#include <cstdio>
#include "l4_types.h"
#include "config.h"
#include "jdb_module.h"
#include "space_index.h"
#include "space.h"
#include "static_init.h"
#include "thread.h"

class Utcb;

class Utcb_m : public Jdb_module
{
public:
  Utcb_m() FIASCO_INIT;
private:
  static L4_uid threadid;
};


static Utcb_m Utcb_m INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

L4_uid Utcb_m::threadid;

IMPLEMENT
Utcb_m::Utcb_m()
  : Jdb_module("INFO")
{ 
}

PUBLIC virtual
Jdb_module::Action_code
Utcb_m::action( int cmd, void *&, char const *&, int &)
{
  if (cmd != 0) 
    return NOTHING;

  Thread *t = Thread::id_to_tcb(threadid);
  if(!t->is_valid())
    {
      printf(" Invalid thread\n");
      return NOTHING;
    }

  printf("\n"
         "current utcb:  "L4_PTR_FMT"\n"
         "utcbaddr:      "L4_PTR_FMT"\n",
         Mem_layout::user_utcb_ptr(),
         (Address)(Mem_layout::V2_utcb_addr + t->id().lthread() * sizeof(Utcb)));
  t->utcb()->print();
  return NOTHING;
}

PUBLIC 
int
Utcb_m::num_cmds() const
{ 
  return 1;
}

PUBLIC
Jdb_module::Cmd
const * Utcb_m::cmds() const
{
  static Cmd cs[] =
    {
      { 0, "z", "z", "%t",
	"z<threadid>\tShow user tcb", &Utcb_m::threadid }
    };
  return cs;
}
