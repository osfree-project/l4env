IMPLEMENTATION[smas]:

#include <cstdio>
#include "config.h"
#include "simpleio.h"
#include "smas.h"
#include "static_init.h"

class Jdb_kern_info_smas : public Jdb_kern_info_module
{
};

static Jdb_kern_info_smas k_s INIT_PRIORITY(JDB_MODULE_INIT_PRIO+1);

PUBLIC
Jdb_kern_info_smas::Jdb_kern_info_smas()
  : Jdb_kern_info_module('s', "Small Spaces")
{
  Jdb_kern_info::register_subcmd(this);
}

#undef offsetof
#define offsetof(TYPE, MEMBER) (((size_t) &((TYPE *)10)->MEMBER) - 10)

static inline NOEXPORT
Space*
Jdb_kern_info_smas::space_of_memspace (Mem_space* s)
{
  return (Space*)(((char*) s) - offsetof(Space, _mem_space));
}

PUBLIC
void
Jdb_kern_info_smas::show()
{
  printf("Small Spaces: %08x-%08x (%dMB",
      Mem_layout::Smas_start, Mem_layout::Smas_end,
      (Mem_layout::Smas_end - Mem_layout::Smas_start) / (1<<20));
  if (smas._space_count < 0)
    puts(", none allocated)");
  else
    printf("=%d*%dMB)\n",
      smas._space_count, smas._space_size*Config::SUPERPAGE_SIZE / (1<<20));

  printf("  kdir version: %08x\n", Kmem::smas_pdir_version());

  Mem_space *last_space = smas._spaces[0];
  printf("  00-");
  for (unsigned i=0, j=0; i<=smas._available_size; i++)
    {
      if (smas._spaces[i]!=last_space || i==smas._available_size)
	{
	  printf("%02x (%3dMB): ", i-1, (i-j)*Config::SUPERPAGE_SIZE/(1<<20));
	  if (last_space)
	    {
    	      printf("%02x  version %08x area %08x-%08x\n", 
		    (unsigned) space_of_memspace(last_space)->id(),
		    last_space->smas_pdir_version(),
	      	    last_space->small_space_base(),
      		    last_space->small_space_base() +
		    last_space->small_space_size());
	    }
	  else
	    printf("--\n");
	  if (i!=smas._available_size)
	    printf("  %02x-", i);
	  last_space = smas._spaces[i];
	  j=i;
	}
    }
}
