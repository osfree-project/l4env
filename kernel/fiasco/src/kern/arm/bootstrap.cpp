INTERFACE [arm]:

#include <cstddef>
#include "types.h"
#include "mem_layout.h"

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include "cpu.h"

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && armv5]:

enum
{
  Section_cachable = 0x40e,
  Section_no_cache = 0x402,
  Section_local    = 0,
  Section_global   = 0,
};

void
set_asid()
{}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && armv6]:

enum
{
  Section_cachable = 0x5406,
  Section_no_cache = 0x0402,
  Section_local    = (1 << 17),
  Section_global   = 0,
};

void
set_asid()
{
  asm volatile ("MCR p15, 0, %0, c13, c0, 1" : : "r" (0)); // ASID 0
}

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

void
map_1mb(void *pd, Address va, Address pa, bool cache, bool local)
{
  Unsigned32 *const p = (Unsigned32*)pd;
  p[va >> 20] = (pa & 0xfff00000)
                | (cache ? Section_cachable : Section_no_cache)
                | (local ? Section_local : Section_global);
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include "kmem_space.h"

enum 
{
  Max_ram_size = 1024*1024*128, // 128MB
};

asm 
(
".section .text.init,#alloc,#execinstr \n"
".global _start                        \n"
"_start:                               \n"
"     ldr sp, __init_data              \n"
"     bl	bootstrap_main         \n"

"__init_data:                          \n"
".long _stack                          \n"
".previous                             \n"
".section .bss                         \n"        
"	.space	2048                   \n"
"_stack:                               \n"
".previous                             \n"
);


#include "mmu.h"

#include "globalconfig.h"

extern char bootstrap_bss_start[];
extern char bootstrap_bss_end[];
extern char __bss_start[];
extern char __bss_end[];

enum 
{
  Virt_ofs = Mem_layout::Sdram_phys_base - Mem_layout::Map_base,
};

extern "C" int bootstrap_main()
{
#if 0 // boot_arm sweaps out the BSS for us
  /* just to make sure the BSS is sweaped clean */
  for (char *p = bootstrap_bss_start; p < bootstrap_bss_end; ++p)
    *p = 0;

  /* just to make sure the BSS is sweaped clean */
  for (char *p = __bss_start + Virt_ofs; p < __bss_end + Virt_ofs; ++p)
    *p = 0;
#endif
  
  extern char kernel_page_directory[];
  void *const page_dir = kernel_page_directory + Virt_ofs;
  
  Address va, pa;
  // map sdram linear from 0xf0000000
  for (va = Mem_layout::Map_base, pa = Mem_layout::Sdram_phys_base;
       va < Mem_layout::Map_base + (4 << 20); va+=0x100000, pa+=0x100000) 
    map_1mb(page_dir, va, pa, true, false);

  // map sdram 1:1
  for (va = Mem_layout::Sdram_phys_base;
       va < Mem_layout::Sdram_phys_base + (4<<20); va+=0x100000) 
    map_1mb(page_dir, va, va, true, true);

  map_hw(page_dir);

  unsigned domains      = 0x55555555; // client for all domains
  unsigned control      = Config::cache_enabled
                          ? Cpu::Cp15_c1_cache_enabled : Cpu::Cp15_c1_cache_disabled;
  //unsigned control        = 0x01003;
  //unsigned control      = 0x00063;

  Mmu<Cache_flush_area, true>::flush_cache();

  extern char _start_kernel[];

  asm volatile (
      "mcr p15, 0, r0, c8, c7, 0x00   \n" // TLB flush
      "mcr p15, 0, %[doms], c3, c0    \n" // domains
      "mcr p15, 0, %[pdir], c2, c0    \n" // pdbr
      "mcr p15, 0, %[control], c1, c0 \n" // control

      "mrc p15, 0, r0, c2, c0, 0      \n" // arbitrary read of cp15
      "mov r0, r0                     \n" // wait for result
      "sub pc, pc, #4                 \n"

      "mov pc, %[start]               \n"
      : :
      [pdir]    "r"(page_dir), 
      [doms]    "r"(domains), 
      [control] "r"(control),
      [start]   "r"(_start_kernel)
      : "r0"
      );
#if 0
  wrb('O');
  extern char __main[];

  asm volatile (" mov pc, %0 \n" :  : "r"(__main) );

//  void (*stk)() = (void (*)())(_start_kernel);

//  stk();
#endif

  while(1);
}

