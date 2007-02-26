IMPLEMENTATION:

#include "banner.h"
#include "boot_info.h"
#include "config.h"
#include "cpu.h"
#include "dirq.h"
#include "fb.h"
#include "fpu.h"
#include "idt.h"
#include "initcalls.h"
#include "irq.h"
#include "jdb.h"
#include "kip_init.h"
#include "kmem.h"
#include "kmem_alloc.h"
#include "pic.h"
#include "static_init.h"
#include "timer.h"
#include "usermode.h"
#include "utcb_init.h"
#include "vmem_alloc.h"

static void startup_system();

STATIC_INITIALIZER_P(startup_system, STARTUP_INIT_PRIO);

static void FIASCO_INIT
startup_system()
{
  Usermode::init();
  Boot_info::init();
  Banner::init();
  Cpu::init();
  Config::init();
  Kmem::init();
  Kmem_alloc::init();
  Kip_init::init();
  Vmem_alloc::init();
  Utcb_init::init();
  Pic::init();
  Idt::init();
  Irq_alloc::init();
  Dirq::init();
  Fpu::init();
  Timer::init();
  Fb::init();
}
