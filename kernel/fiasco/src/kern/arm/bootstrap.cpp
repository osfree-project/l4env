INTERFACE [arm]:

#include <cstddef>
#include "types.h"
#include "mem_layout.h"

//---------------------------------------------------------------------------
IMPLEMENTATION [arm && armv5]:

enum
{
  Section_cachable = 0x40e,
  Section_no_cache = 0x402,
  Cp15_c1 = 0x0100f,
};


//---------------------------------------------------------------------------
IMPLEMENTATION [arm && armv6]:

enum
{
  Section_cachable = 0x140e,
  Section_no_cache = 0x0402,
  Cp15_c1 = 0x803007,
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

void 
map_1mb(void *pd, Address va, Address pa, bool cache)
{
  Unsigned32 *const p = (Unsigned32*)pd;
  p[va >> 20] = (pa & 0xfff00000) | (cache?Section_cachable:Section_no_cache);
}

//---------------------------------------------------------------------------
IMPLEMENTATION[arm && sa1100]:

enum {
  Cache_flush_area = 0xe0000000,
};

static
void
map_hw(void *pd)
{
  // map the cache flush area to 0xef000000
  map_1mb(pd, Mem_layout::Cache_flush_area, Mem_layout::Flush_area_phys_base, true);
  // map UART
  map_1mb(pd, Mem_layout::Uart_map_base, Mem_layout::Uart_phys_base, false);
  map_1mb(pd, Mem_layout::Uart_phys_base, Mem_layout::Uart_phys_base, false);
  // map Timer and Pic
  map_1mb(pd, Mem_layout::Timer_map_base, Mem_layout::Timer_phys_base, false);
}

static inline
void wrb(char c)
{
  *((volatile Unsigned8*)(Mem_layout::Uart_phys_base + 0x14)) = c;
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && pxa]: 

enum {
  Cache_flush_area = 0xa0100000, // XXX: hacky
};

static
void
map_hw(void *pd)
{
  // map the cache flush area to 0xef000000
  map_1mb(pd, Mem_layout::Cache_flush_area, Mem_layout::Flush_area_phys_base, true);
  // map UART
  map_1mb(pd, Mem_layout::Uart_map_base, Mem_layout::Uart_phys_base, false);
  map_1mb(pd, Mem_layout::Uart_phys_base, Mem_layout::Uart_phys_base, false);
  // map Timer
  map_1mb(pd, Mem_layout::Timer_map_base, Mem_layout::Timer_phys_base, false);
  // map Pic
  map_1mb(pd, Mem_layout::Pic_map_base, Mem_layout::Pic_phys_base, false);
}

static inline
void wrb(char c)
{
  *((volatile Unsigned8*)(Mem_layout::Uart_phys_base)) = c;
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && integrator]:

enum {
  Cache_flush_area = 0xe0000000,
};

static
void
map_hw(void *pd)
{
  // map UART
  map_1mb(pd, Mem_layout::Uart_map_base, Mem_layout::Uart_phys_base, false);
  // map Timer
  map_1mb(pd, Mem_layout::Timer_map_base, Mem_layout::Timer_phys_base, false);
  // map Pic
  map_1mb(pd, Mem_layout::Pic_map_base, Mem_layout::Pic_phys_base, false);
  // map Integrator hdr
  map_1mb(pd, Mem_layout::Integrator_map_base, Mem_layout::Integrator_phys_base, false);
}

static inline
void wrb(char c)
{
  *((volatile Unsigned8*)(Mem_layout::Uart_phys_base)) = c;
}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && realview && mpcore]:
static void map_hw2(void *pd)
{ map_1mb(pd, Mem_layout::Mpcore_scu_map_base, Mem_layout::Mpcore_scu_phys_base, false); }

IMPLEMENTATION [arm && realview && !mpcore]:
static void map_hw2(void *pd)
{}

IMPLEMENTATION [arm && realview]:

enum {
  Cache_flush_area = 0,
};

static
void
map_hw(void *pd)
{
  // map devices
  map_1mb(pd, Mem_layout::Devices_map_base, Mem_layout::Devices_phys_base, false);
  map_hw2(pd);
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
    map_1mb(page_dir, va, pa, true);

  // map sdram 1:1
  for (va = Mem_layout::Sdram_phys_base;
       va < Mem_layout::Sdram_phys_base + (4<<20); va+=0x100000) 
    map_1mb(page_dir, va, va, true);


  map_hw(page_dir);

  unsigned domains      = 0x55555555; // client for all domains
  unsigned control      = Cp15_c1;
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

