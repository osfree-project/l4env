IMPLEMENTATION [ux]:

#include <cstdio>
#include "cpu.h"
#include "simpleio.h"
#include "space.h"


class Jdb_kern_info_misc : public Jdb_kern_info_module
{
};

static Jdb_kern_info_misc k_i INIT_PRIORITY(JDB_MODULE_INIT_PRIO+1);

PUBLIC
Jdb_kern_info_misc::Jdb_kern_info_misc()
  : Jdb_kern_info_module('i', "Miscellaneous info")
{
  Jdb_kern_info::register_subcmd(this);
}

PUBLIC
void
Jdb_kern_info_misc::show()
{
  printf ("clck: %08x.%08x\n",
	  (unsigned) (Kip::k()->clock >> 32), 
	  (unsigned) (Kip::k()->clock));
  show_pdir();
}


class Jdb_kern_info_cpu : public Jdb_kern_info_module
{
};

static Jdb_kern_info_cpu k_c INIT_PRIORITY(JDB_MODULE_INIT_PRIO+1);

PUBLIC
Jdb_kern_info_cpu::Jdb_kern_info_cpu()
  : Jdb_kern_info_module('c', "CPU features")
{
  Jdb_kern_info::register_subcmd(this);
}

PUBLIC
void
Jdb_kern_info_cpu::show()
{
  char cpu_mhz[32];
  unsigned hz;

  cpu_mhz[0] = '\0';
  if ((hz = Cpu::frequency()))
    {
      unsigned mhz = hz / 1000000;
      hz -= mhz * 1000000;
      unsigned khz = hz / 1000;
      snprintf(cpu_mhz, sizeof(cpu_mhz), "%d.%03d MHz", mhz, khz);
    }

  printf ("CPU: %s %s\n", Cpu::model_str(), cpu_mhz);
  Cpu::show_cache_tlb_info("     ");
  show_features();
}

