IMPLEMENTATION [arm]:

#include "boot_info.h"
#include "config.h"
#include "cpu.h"
#include "dirq.h"
#include "irq_alloc.h"
#include "kern_lib_page.h"
#include "kernel_task.h"
#include "kip_init.h"
#include "kmem_alloc.h"
#include "kmem_space.h"
#include "pic.h"
#include "processor.h"
#include "static_init.h"
#include "timer.h"
#include "utcb_init.h"
#include "vmem_alloc.h"

#include <cstdlib>
#include <cstdio>

IMPLEMENT FIASCO_INIT FIASCO_NOINLINE
void 
Startup::stage1() 
{
  Proc::cli();
  Boot_info::init();
  Cpu::early_init();
  Config::init();
  Config::serial_esc = Config::SERIAL_ESC_IRQ;
  Irq_alloc::init();
  Dirq::init();
}

IMPLEMENT FIASCO_INIT FIASCO_NOINLINE 
void 
Startup::stage2()
{
  puts("Hello from Startup::stage2");
  // The first 4MB of phys memory are always mapped to Map_base
  Mem_layout::add_pmem(Mem_layout::Sdram_phys_base, Mem_layout::Map_base,
      4<<20);
  Kip_init::init();
  Kmem_alloc::init();
  Kmem_space::init();
  Kernel_task::init();
  Pic::init();
  Vmem_alloc::init();
  Cpu::init();
  Timer::init();
  Kern_lib_page::init();
  Utcb_init::init();
}
