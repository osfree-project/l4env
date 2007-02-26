IMPLEMENTATION[ia32-ux]:

#include <cstdio>
#include <cstring>
#include "simpleio.h"

//#include "globals.h"
//#include "space.h"
//#include "kmem.h"

PUBLIC
void
Jdb_kern_info_cpu::show_f_bits (unsigned features, const char *const *table,
				unsigned first_pos, unsigned &last_pos,
				unsigned &colon)
{
  unsigned i, count;

  for (i=count=0; *table != (char *) -1; i++, table++)
    if ((features & (1 << i)) && *table)
      {
	int slen = strlen(*table);
	if (last_pos+colon+slen > 78)
	  {
	    colon = 0;
	    last_pos = first_pos;
	    printf("\n%*s", first_pos, "");
	  }
	printf ("%s%s", colon ? ", " : "", *table);
	last_pos += slen + colon;
	colon = 2;
      }
}

PUBLIC
void
Jdb_kern_info_cpu::show_features()
{
  const char *const simple[] = 
  {
    "fpu (fpu on chip)",
    "vme (virtual-8086 mode enhancements)",
    "de (I/O breakpoints)",
    "pse (4MB pages)",
    "tsc (rdtsc instruction)",
    "msr (rdmsr/rdwsr instructions)",
    "pae (physical address extension)",
    "mce (machine check exception #18)",
    "cx8 (cmpxchg8 instruction)",
    "apic (on-chip APIC)",
    NULL,
    "sep (sysenter/sysexit instructions)",
    "mtrr (memory type range registers)",
    "pge (global TLBs)",
    "mca (machine check architecture)",
    "cmov (conditional move instructions)",
    "pat (page attribute table)",
    "pse36 (32-bit page size extension)",
    "psn (processor serial number)",
    "clfsh (flush cache line instruction)",
    NULL,
    "ds (debug store to memory)",
    "acpi (thermal monitor and soft controlled clock)",
    "mmx (MMX technology)",
    "fxsr (fxsave/fxrstor instructions)",
    "sse (SSE extensions)",
    "sse2 (SSE2 extensions)",
    "ss (self snoop of own cache structures)",
    "htt (hyper-threading technology)",
    "tm (thermal monitor)",
    NULL,
    "pbe (pending break enable)",
    (char *)(-1)
  };
  const char *const extended[] =
  {
    "pni (prescott new instructions)",
    NULL,
    NULL,
    "monitor (monitor/mwait instructions)",
    "dscpl (CPL qualified debug store)",
    NULL,
    NULL,
    "est (enhanced speedstep technology)",
    "tm2 (thermal monitor 2)",
    NULL,
    "cid (L1 context id)",
    (char *)(-1)
  };

  unsigned position = 5, colon = 0;
  putstr("     ");
  show_f_bits (Cpu::features(), simple, 5, position, colon);
  show_f_bits (Cpu::ext_features(), extended, 5, position, colon);
}


class Jdb_kern_info_memory : public Jdb_kern_info_module
{
};

static Jdb_kern_info_memory k_m INIT_PRIORITY(JDB_MODULE_INIT_PRIO+1);

PUBLIC
Jdb_kern_info_memory::Jdb_kern_info_memory()
  : Jdb_kern_info_module('m', "kmem_alloc::debug_dump")
{
  Jdb_kern_info::register_subcmd(this);
}

PUBLIC
void
Jdb_kern_info_memory::show()
{
  Kmem_alloc::allocator()->debug_dump();
}


class Jdb_kern_info_region : public Jdb_kern_info_module
{
};

static Jdb_kern_info_region k_r INIT_PRIORITY(JDB_MODULE_INIT_PRIO+1);

PUBLIC
Jdb_kern_info_region::Jdb_kern_info_region()
  : Jdb_kern_info_module('r', "region::debug_dump")
{
  Jdb_kern_info::register_subcmd(this);
}

PUBLIC
void
Jdb_kern_info_region::show()
{
  region::debug_dump();
}


class Jdb_kern_info_kip : public Jdb_kern_info_module
{
};

static Jdb_kern_info_kip k_f INIT_PRIORITY(JDB_MODULE_INIT_PRIO+1);

PUBLIC
Jdb_kern_info_kip::Jdb_kern_info_kip()
  : Jdb_kern_info_module('f', "show kernel interface page")
{
  Jdb_kern_info::register_subcmd(this);
}

PUBLIC
void
Jdb_kern_info_kip::show()
{
  Kmem::info()->print();
}

