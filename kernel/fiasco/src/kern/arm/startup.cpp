IMPLEMENTATION:

#include "boot_console.h"
#include "boot_info.h"
#include "config.h"
#include "cpu.h"
#include "dirq.h"
#include "irq_alloc.h"
#include "kip_init.h"
#include "kmem_space.h"
#include "static_init.h"
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
  Boot_console::init();
  Vmem_alloc::init();
  Irq_alloc::init();
  dirq_t::init();
  Cpu::init();
}
