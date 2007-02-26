IMPLEMENTATION:

#include "boot_console.h"
#include "boot_info.h"
#include "config.h"
#include "cpu.h"
#include "dirq.h"
#include "irq_alloc.h"
#include "kip_init.h"
#include "kmem_space.h"
#include "pic.h"
#include "static_init.h"
#include "timer.h"
#include "vmem_alloc.h"

#include <cstdlib>
#include <cstdio>

static void startup_system();

STATIC_INITIALIZER_P(startup_system, STARTUP_INIT_PRIO);

static void startup_system()
{
  Boot_info::init();
  Cpu::early_init();

  printf("Hello from the kernel\n");

  Config::init();
  Kip::init();
  Kmem_space::init();
  Pic::init();
#if !defined(CONFIG_KDB)
  Boot_console::init();
#endif
  Vmem_alloc::init();
  Irq_alloc::init();
  Dirq::init();
  Cpu::init();
  Timer::init();
}
