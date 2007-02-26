IMPLEMENTATION:

#include "boot_console.h"
#include "boot_info.h"
#include "config.h"
#include "cpu.h"
#include "dirq.h"
#include "fpu.h"
#include "idt.h"
#include "initcalls.h"
#include "irq_alloc.h"
#include "kmem.h"
#include "pic.h"
#include "static_init.h"
#include "timer.h"
#include "vmem_alloc.h"

static void startup_system() FIASCO_INIT FIASCO_NOINLINE;

STATIC_INITIALIZER_P(startup_system, STARTUP_INIT_PRIO);

static void startup_system() 
{
  Boot_info::init();
  Boot_console::init();
  Cpu::init();
  Pic::init();
  Config::init();
  Kmem::init();
  Vmem_alloc::init();
  Idt::init();
  Irq_alloc::init();
  dirq_t::init();
  Fpu::init();
  Timer::init();
}
