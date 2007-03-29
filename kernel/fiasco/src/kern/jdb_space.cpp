IMPLEMENTATION:

#include <climits>
#include <cstring>
#include <cstdio>

#include "jdb.h"
#include "jdb_core.h"
#include "jdb_module.h"
#include "jdb_screen.h"
#include "kernel_console.h"
#include "keycodes.h"
#include "ram_quota.h"
#include "simpleio.h"
#include "space.h"
#include "static_init.h"

class Jdb_space : public Jdb_module
{
public:
  Jdb_space() FIASCO_INIT;
private:
  static unsigned space_num;
};

unsigned Jdb_space::space_num;

IMPLEMENT
Jdb_space::Jdb_space()
  : Jdb_module("INFO")
{}

PUBLIC
Jdb_module::Action_code
Jdb_space::action(int cmd, void *&, char const *&, int &)
{
  if (cmd == 0)
    {
      Space *s = Space_index(space_num).lookup();
      if (!s)
	{
	  printf("Invalid space number!\n");
	  return NOTHING;
	}

      printf("Space %3x @%p\n", space_num, s);
      printf("  page table: %p\n", s->mem_space());
      cap_space_info(s);
      io_space_info(s);
      unsigned long m = s->ram_quota()->current();
      unsigned long l = s->ram_quota()->limit();
      printf("  mem usage:  %ld (%ldKB) of %ld (%ldKB) @%p\n", 
	  m, m/1024, l, l/1024, s->ram_quota());
      return NOTHING;
    }
  return NOTHING;
}

PUBLIC
Jdb_module::Cmd const *
Jdb_space::cmds() const
{
  static Cmd cs[] =
    {
	{ 0, "s", "space", "%x\n", "<task_num>\t show task information", 
	  &space_num },
    };
  return cs;
}
  
PUBLIC
int
Jdb_space::num_cmds() const
{ return 1; }

static Jdb_space jdb_space INIT_PRIORITY(JDB_MODULE_INIT_PRIO);


IMPLEMENTATION[!io]:

PRIVATE
void
Jdb_space::io_space_info(Space *)
{}

IMPLEMENTATION[!caps]:

PRIVATE
void
Jdb_space::cap_space_info(Space *)
{}

IMPLEMENTATION[io]:

PRIVATE
void
Jdb_space::io_space_info(Space *s)
{
  printf("  io_space:   %p\n", s->io_space());
}

IMPLEMENTATION[caps]:

PRIVATE
void
Jdb_space::cap_space_info(Space *s)
{
  printf("  cap_space:  %p\n", s->cap_space());
}

