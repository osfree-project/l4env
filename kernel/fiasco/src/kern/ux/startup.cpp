IMPLEMENTATION:

#include "boot_info.h"
#include "config.h"
#include "cpu.h"
#include "dirq.h"
#include "fpu.h"
#include "idt.h"
#include "initcalls.h"
#include "irq.h"
#include "kmem.h"
#include "pic.h"
#include "static_init.h"
#include "timer.h"
#include "usermode.h"
#include "vmem_alloc.h"

static void startup_system();

STATIC_INITIALIZER_P(startup_system, STARTUP_INIT_PRIO);

static void FIASCO_INIT
startup_system()
{
  Usermode::init();
  Boot_info::init();
  Cpu::init();
  Pic::init();
  Config::init();
  Kmem::init();
  Vmem_alloc::init();
  Idt::init();
  irq_t::init();
  dirq_t::init();
  Fpu::init();
  Timer::init();
}
