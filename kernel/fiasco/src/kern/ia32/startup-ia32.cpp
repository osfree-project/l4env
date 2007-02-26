IMPLEMENTATION:

#include <cstdlib>
#include <cstdio>

#include "apic.h"
#include "banner.h"
#include "boot_console.h"
#include "boot_info.h"
#include "config.h"
#include "cpu.h"
#include "dirq.h"
#include "fpu.h"
#include "idt.h"
#include "initcalls.h"
#include "irq_alloc.h"
#include "kernel_console.h"
#include "kip_init.h"
#include "kmem.h"
#include "kmem_alloc.h"
#include "pic.h"
#include "static_init.h"
#include "std_macros.h"
#include "thread.h"
#include "timer.h"
#include "utcb_init.h"
#include "vmem_alloc.h"
#include "space_index.h"

IMPLEMENT FIASCO_INIT FIASCO_NOINLINE
void
Startup::stage1()
{
  Boot_info::init();
  Config::init();
  Irq_alloc::init();
  Dirq::init();
}

IMPLEMENT FIASCO_INIT FIASCO_NOINLINE
void
Startup::stage2()
{
  Banner::init();
  Cpu::init();
  Pic::init();
  Kmem::init();
  Kmem_alloc::init();
  Kip_init::init();
  Vmem_alloc::init();
  Utcb_init::init();
  Idt::init();
  Fpu::init();
  Apic::init();
  Timer::init();
  Apic::check_still_getting_interrupts();
}
