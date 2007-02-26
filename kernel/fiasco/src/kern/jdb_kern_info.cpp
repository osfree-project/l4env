IMPLEMENTATION:

#include <cstdio>

#include "cpu.h"
#include "jdb.h"
#include "jdb_module.h"
#include "static_init.h"
#include "kmem_alloc.h"
#include "region.h"


//===================
// Std JDB modules
//===================

class Jdb_kern_info_module
{
  friend class Jdb_kern_info;
  virtual void show(void) = 0;

private:
  char                 _subcmd;
  char const           *_descr;
  Jdb_kern_info_module *_next;
};

PUBLIC
Jdb_kern_info_module::Jdb_kern_info_module(char subcmd, char const *descr)
{
  _subcmd = subcmd;
  _descr  = descr;
}

/**
 * @brief 'kern info' module.
 * 
 * This module handles the 'k' command, which
 * prints out various kernel information.
 */
class Jdb_kern_info : public Jdb_module
{
private:
  static char                 _subcmd;
  static Jdb_kern_info_module *_first;
};

char Jdb_kern_info::_subcmd;
Jdb_kern_info_module *Jdb_kern_info::_first;

PUBLIC static
void
Jdb_kern_info::register_subcmd(Jdb_kern_info_module *m)
{
  Jdb_kern_info_module *kim = _first;
  Jdb_kern_info_module *kim_last = 0;

  while (kim && kim->_subcmd < m->_subcmd)
    {
      kim_last = kim;
      kim = kim->_next;
    }
  if (kim_last)
    kim_last->_next = m;
  else
    _first = m;
  m->_next = kim;
}

PUBLIC
Jdb_module::Action_code
Jdb_kern_info::action( int cmd, void *&args, char const *&, int & )
{
  if(cmd!=0)
    return NOTHING;
  
  char c = *(char*)(args);
  Jdb_kern_info_module *kim;

  kim = _first;
  while (kim)
    {
      if (kim->_subcmd == c)
	{
	  kim->show();
	  putchar('\n');
	  return NOTHING;
	}
      kim = kim->_next;
    }

  kim = _first;
  while (kim)
    {
      printf("  k%c   %s\n", kim->_subcmd, kim->_descr);
      kim = kim->_next;
    }

  putchar('\n');
  return NOTHING;
}

PUBLIC
int const
Jdb_kern_info::num_cmds() const
{ 
  return 1;
}

PUBLIC
Jdb_module::Cmd const *const
Jdb_kern_info::cmds() const
{
  static Cmd cs[] =
    {
      Cmd( 0, "k", "k", "%c\n", 
	   "k\tshow various kernel information (kh=help)", &_subcmd )
    };

  return cs;
}

PUBLIC
Jdb_kern_info::Jdb_kern_info()
  : Jdb_module("INFO")
{}

static Jdb_kern_info jdb_kern_info INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

