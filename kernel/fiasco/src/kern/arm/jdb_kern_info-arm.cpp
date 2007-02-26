IMPLEMENTATION[arm]:

#include <cstdio>
#include <cstring>

#include "config.h"
#include "globals.h"
#include "kmem.h"
#include "space.h"

class Jdb_kern_info_misc : public Jdb_kern_info_module
{
};

static Jdb_kern_info_misc k_i INIT_PRIORITY(JDB_MODULE_INIT_PRIO+1);

PUBLIC
Jdb_kern_info_misc::Jdb_kern_info_misc()
  : Jdb_kern_info_module('i', "miscellanous info")
{
  Jdb_kern_info::register_subcmd(this);
}

PUBLIC
void
Jdb_kern_info_misc::show()
{
  Space *s = current_space();

  printf("clck: %08x.%08x\n"
	 "pdir: %08x (taskno=%03x, chief=%03x)\n",
	 (unsigned) (Kmem::info()->clock >> 32), 
	 (unsigned) (Kmem::info()->clock),
	 (unsigned) s,
	 unsigned(s->space()),  unsigned(s->chief()));
}

