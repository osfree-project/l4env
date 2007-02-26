INTERFACE [arm]:

#include <cstddef>
#include "types.h"
#include "kmem.h"

//---------------------------------------------------------------------------
IMPLEMENTATION[arm-sa1100]:

enum {
  Cache_flush_area = 0xe0000000,
};


IMPLEMENTATION [arm-pxa]: 

enum {
  Cache_flush_area = 0xa0100000, // XXX: hacky
};


IMPLEMENTATION [arm]:

#include <cstdio>
#include <cstring>

#include "load_elf.h"
#include "kip.h"
#include "h3xxx.h"
#include "mmu.h"

#include "static_init.h"
#include "globalconfig.h"

STATIC_INITIALIZER_P(console_init,MAX_INIT_PRIO);

IMPLEMENTATION[arm-xgdb]:

#include "mux_console.h"

void console_init()
{
  static Mux_console con;
  Console::stdout= &con;
  Console::stderr= &con;
  Console::stdin = &con;
}
IMPLEMENTATION[arm-sa1100]:

#include "uart.h"

void console_init()
{
  static Uart sercon;
  //  static Gdb_serv gdb(&sercon);
  static Uart &gdb = sercon;

  H3xxx_generic< 0x49000000 >::h3800_init();
  
  sercon.startup(0x80050000,17);
  sercon.change_mode(Uart::MODE_8N1, 115200);
  Console::stdout= &gdb;
  Console::stderr= &gdb;
  Console::stdin = &gdb;
}

IMPLEMENTATION[arm-pxa]:

#include "uart.h"

void console_init()
{
  static Uart sercon;
  //  static Gdb_serv gdb(&sercon);
  static Uart &gdb = sercon;

  sercon.startup(0x40100000/4,22);
  sercon.change_mode(Uart::MODE_8N1, 115200);
  Console::stdout= &gdb;
  Console::stderr= &gdb;
  Console::stdin = &gdb;
}


IMPLEMENTATION [arm]:

Unsigned32 *page_dir;

extern "C" char _binary_kernel_start;
extern "C" char _binary_sigma0_start;
extern "C" char _binary_test_start;

int main(Address s, Address r)
{
  
  // caution: no stack variables in this function because we're going
  // to change the stack pointer!

  // make some basic initializations, then create and run the kernel
  // thread
  
  // console initialization
  //  switch_vid(console::use_hercules);
 
  printf("LOADER: Hello I'm there!\n");

  printf("LOADER: Setting up a really small pagetable and start the kernel\n");

#warning BAD HARDCODED THINGS FOLLOW
  char *scratch_mem = (char*)(Kmem::Sdram_phys_base|0x40000);
  unsigned scratch_mem_size = 0x05000;

  // bad hardcoded things !!
  page_dir = (Unsigned32*)scratch_mem; // assume the bootloader is less than 64KB
  memset(page_dir, 0, 4096*4);

  Kip *kip = (Kip*)(scratch_mem+0x4000); // right behind the page dir
  memset(kip,0,4096);

  kip->offset_version_strings = (sizeof(Kip) +30*sizeof(Mem_desc)) >> 4;

  kip->init_mem_info(sizeof(Kip), 20);

  // add 64MB conventional memory
  kip->add_mem_region(Mem_desc(Kmem::Sdram_phys_base, 
	                       Kmem::Sdram_phys_base + 64*1024*1024 -1,
			       Mem_desc::Conventional));

  // reserve bootloader scratch mem
  kip->add_mem_region(Mem_desc((Address)scratch_mem,
	                       (Address)scratch_mem + scratch_mem_size -1,
			       Mem_desc::Bootloader));

  // the kernel is assumed to be loaded @ 0xc1100000 and mapped to 0xc0000000
  //  page_dir[0xc00]= 0xc1100c0e; // 1MB section privileged rw domain 0 (cb)
  
#ifdef CONFIG_ARM_SA  // XXX: only SA
  page_dir[0x800]= 0x80000c02; // 1MB section for Uart regs rw domain 0 (ncnb)
#endif

#ifdef CONFIG_ARM_PXA
  page_dir[0x401]= 0x40100c02; // 1MB section for Uart regs
#endif

  // map sdram linear from 0xf0000000
  for( unsigned pe= Kmem::Map_base >> 20, va=Kmem::Sdram_phys_base;
       pe < (Kmem::Map_base >> 20) + 0xff; pe++,va+=0x100000 ) 
    page_dir[pe]=va | 0x0c0e;

  // map sdram linear from 0xc0000000
  for( unsigned pe=Kmem::Sdram_phys_base>>20, va=Kmem::Sdram_phys_base; 
       pe < ((Kmem::Sdram_phys_base+128*1024*1024)>>20); 
       pe++,va+=0x100000 ) 
    page_dir[pe]=va | 0x0c0e;
  
  // map the cache flush area to 0xef000000
  page_dir[Kmem::Cache_flush_area>>20] = Kmem::Flush_area_phys_base | 0x0c0e;

  unsigned domains      = 0x55555555; // client for all domains
  unsigned control      = 0x0100f;

  void (*start)(Kip*);
#if 0
  Startup_func sigma0, rmgr;
#endif
  Mword k_start, k_end;

  start = (void (*)(Kip*))load_elf("Fiasco", &_binary_kernel_start, &k_start, &k_end);
  if(!start)
    {
      printf("LOADER: error: start address of 0 is not allowed, just halt\n");
      while(1);
    }

  kip->sigma0_pc = s;
  kip->root_pc   = r;

  Mmu<Cache_flush_area>::write_back_data_cache();

  printf("LOADER: establish kernel mappings\nLOADER: Start kernel @%p\n",
	 start);

  asm volatile ( "mcr p15, 0, r0, c7, c7, 0x00   \n" // Cache flush
		 "mcr p15, 0, r0, c8, c7, 0x00   \n" // TLB flush
                 "mcr p15, 0, %[doms], c3, c0    \n" // domains
                 "mcr p15, 0, %[pdir], c2, c0    \n" // pdbr
                 "mcr p15, 0, %[control], c1, c0 \n" // control
		 
		 "mrc p15, 0, r0, c2, c0, 0      \n" // arbitrary read of cp15
		 "mov r0, r0                     \n" // wait for result
		 "sub pc, pc, #4                 \n"
		 
		 "mov r0, %[kip]                 \n"
		 "mov pc, %[start]               \n"
		 : :
		 [pdir]    "r"(page_dir), 
		 [doms]    "r"(domains), 
		 [control] "r"(control),
		 [start]   "r"(start),
		 [kip]     "r"(kip)
		 : "r0"
		);

  printf("Oooh: LOADER must not reach this statement (HALT)\n");
  
  // We should never reach here, because the assembler 
  // in before jumps to the kernel entry point @0xc0000000
  while(1);
}

