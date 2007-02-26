INTERFACE:
#include <cstddef>

#include "types.h"
IMPLEMENTATION:

#include <cstdio>
#include <cstring>

#include "load_elf.h"
#include "uart.h"
#include "gdb_server.h"
#include "kip.h"

#include "static_init.h"
STATIC_INITIALIZER_P(console_init,MAX_INIT_PRIO);

static inline
void write_back_data_cache()
{
  volatile Unsigned32 *base = (Unsigned32*)0xe0000000;
  Unsigned32 *const end = (Unsigned32*)(0xe0000000 + 8192);
  Unsigned32 dummy;
  do {
    dummy = *base;  
    base+=8;
  } while(base!=end) ;
}



void console_init()
{
  static Uart sercon;
  //  static Gdb_serv gdb(&sercon);
  static Uart &gdb = sercon;
  sercon.startup(0x80050000,17);
  sercon.change_mode(Uart::MODE_8N1, 115200);
  Console::stdout= &gdb;
  Console::stderr= &gdb;
  Console::stdin = &gdb;
}


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
  char *scratch_mem = (char*)0xc0040000;
  unsigned scratch_mem_size = 0x05000;

  // bad hardcoded things !!
  page_dir = (Unsigned32*)scratch_mem; // assume the bootloader is less than 64KB
  memset(page_dir, 0, 4096*4);

  Kernel_info *kip = (Kernel_info*)(scratch_mem+0x4000); // right behind the page dir
  memset(kip,0,4096);

  kip->offset_version_strings = (sizeof(Kernel_info) +15) >> 4;
  kip->offset_memory_descs =  (sizeof(Kernel_info) + 271) >> 4;

  Kernel_memory_desc *md = (Kernel_memory_desc*)((char*)kip + 
						 (kip->offset_memory_descs << 4));


  // mark all memory up to the (boot) page table as avialable
  md[0].base = (void*)0xc0000000;
  md[0].size_type = ((char*)scratch_mem - (char*)md[0].base) | L4_MEMORY_DESC_AVAIL; 

  // mark (boot) page table as boot memory and kip
  md[1].base = scratch_mem;
  md[1].size_type = scratch_mem_size | L4_MEMORY_DESC_BOOT;

  md[2].base = (void*)(scratch_mem + 0x5000);
  md[2].size_type = (64*1024*1024) - ((unsigned)scratch_mem + 0x5000 - 0xc0000000);


  // list termination
  md[3].base = 0;
  md[3].size_type = 0;

  kip->total_ram = 64*1024*1024;
  
      

  // the kernel is assumed to be loaded @ 0xc1100000 and mapped to 0xc0000000
  //  page_dir[0xc00]= 0xc1100c0e; // 1MB section privileged rw domain 0 (cb)
  page_dir[0x800]= 0x80000c02; // 1MB section for Uart regs rw domain 0 (ncnb)

  // map sdram linear from 0xf0000000
  for( unsigned pe= 0xf00, va=0xc0000000; pe <0x0fff; pe++,va+=0x100000 ) 
    page_dir[pe]=va | 0x0c0e;

// map sdram linear from 0xc0000000
  for( unsigned pe= 0xc00, va=0xc0000000; pe <0x0cff; pe++,va+=0x100000 ) 
    page_dir[pe]=va | 0x0c0e;

  // map the cache flush area to 0xef000000
  for( unsigned pe= 0xef0, va=0xe0000000; pe <= 0x0ef8; pe++,va+=0x100000 ) 
    page_dir[pe]=va | 0x0c0e;


  unsigned domains      = 0x0001;
  unsigned control      = 0x0100f;

  void (*start)(Kernel_info*);
#if 0
  Startup_func sigma0, rmgr;
#endif
  Mword k_start, k_end, s_start, s_end, r_start, r_end;

  start = (void (*)(Kernel_info*))load_elf("Fiasco", &_binary_kernel_start, &k_start, &k_end);
  if(!start)
    {
      printf("LOADER: error: start address of 0 is not allowed, just halt\n");
      while(1);
    }

#if 0
  sigma0 = load_elf("Sigma0", &_binary_sigma0_start, &s_start, &s_end);
  rmgr   = load_elf("Root", &_binary_test_start, &r_start, &r_end);
#endif

  kip->sigma0_pc = s;
  kip->root_pc   = r;

  write_back_data_cache();

  printf("LOADER: establish kernel mappings\nLOADER: Start kernel @%p\n",
	 start);

  asm volatile ( "mcr p15, 0, %[doms], c3, c0    \n" // domains
                 "mcr p15, 0, %[pdir], c2, c0    \n" // pdbr
                 "mcr p15, 0, %[control], c1, c0 \n" // control
		 "mcr p15, 0, r0, c7, c7, 0x00   \n" // Cache flush
		 "mcr p15, 0, r0, c8, c7, 0x00   \n" // TLB flush
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
  
  // We should never reach here, because the assembler 
  // in before jumps to the kernel entry point @0xc0000000
  while(1);

}
