INTERFACE [arm-x0]:

#include "types.h"
#if 0
enum {
  L4_MEMORY_DESC_TYPE_MASK = 0x00ff,
  L4_MEMORY_DESC_SIZE_MASK = 0xffffff00,

  L4_MEMORY_DESC_AVAIL     = 0x00,
  L4_MEMORY_DESC_BOOT      = 0x01,
  L4_MEMORY_DESC_RESERVED  = 0x02,
};

struct Kernel_memory_desc
{
  void   *base; // base address
  Mword size_type; // in Bytes
};
#endif

EXTENSION class Kip
{
public:

  Mword magic;
  Mword version;
  Mword offset_version_strings;
  Mword _mem_info; //offset_memory_descs;

  /* 0x10 */
  Mword _res1[4];


  /* the following stuff is undocumented; we assume that the kernel
     info page is located at offset 0x1000 into the L4 kernel boot
     image so that these declarations are consistent with section 2.9
     of the L4 Reference Manual */

  /* 0x20 */
  Mword sigma0_sp, sigma0_pc;
  Mword _res2[2];

  /* 0x30 */
  Mword sigma1_sp, sigma1_pc;
  Mword _res3[2];

  /* 0x40 */
  Mword root_sp,   root_pc;
  Mword _res4[2];

  /* 0x50 */
  Mword l4_config;
  Mword _res5;
  Mword kdebug_config;
  Mword kdebug_permission;

  /* 0x60 */
  Mword total_ram;
  Mword _res6[15];

  /* 0xA0 */
  volatile Cpu_time clock;
  Mword _res7[2];

  /* 0xB0 */
  Mword frequency_cpu;
  Mword frequency_bus;
  Mword _res8[2];

  /* 0xC0 */
  
//  Mem_desc _mem[];
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm-x0]:

#include "l4_types.h"

IMPLEMENT inline
Address Kip::main_memory_high() const
{ 
  return 64 << 20;
}

IMPLEMENT
char const *Kip::version_string() const
{
  return reinterpret_cast <char const *> (this) + (offset_version_strings << 4);
}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm-debug-x0]:

#include <cstdio>

static char *types[] = {
    "Undefined",
    "Conventional",
    "Reserved",
    "Dedicated",
    "Shared",
    "(undef)",
    "(undef)",
    "(undef)",
    "(undef)",
    "(undef)",
    "(undef)",
    "(undef)",
    "(undef)",
    "(undef)",
    "Bootloader",
    "Arch" };

IMPLEMENT
void Kip::print() const
{
  
  printf("magic: %.4s  version: 0x%lx\n", (char*)&magic, version);
  printf("sigma0 pc: "L4_PTR_FMT";   sp: "L4_PTR_FMT"\n"
	 "sigma1 pc: "L4_PTR_FMT";   sp: "L4_PTR_FMT"\n"
	 "root   pc: "L4_PTR_FMT";   sp: "L4_PTR_FMT"\n",

	 L4_PTR_ARG(sigma0_pc), L4_PTR_ARG(sigma0_sp),
	 L4_PTR_ARG(sigma1_pc), L4_PTR_ARG(sigma1_sp),
	 L4_PTR_ARG(root_pc), L4_PTR_ARG(root_sp)
	 );

  printf("clock: " L4_X64_FMT "\n", clock);

  printf("Memory (max %d descriptors):\n",num_mem_descs());
  Mem_desc const *m = mem_descs();
  Mem_desc const *const e = m + num_mem_descs();
  for (;m<e;++m)
    {
      if (m->type() != Mem_desc::Undefined)
	printf(" %2d: %s [%08lx-%08lx] %s\n", m - mem_descs() + 1, 
	       m->is_virtual()?"virt":"phys", m->start(), 
	       m->end(), types[m->type()]);
    }

}

