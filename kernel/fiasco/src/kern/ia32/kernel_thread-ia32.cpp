
INTERFACE:

IMPLEMENTATION[ia32]:

#include <flux/x86/base_trap.h>

#include "apic.h"
#include "boot_info.h"
#include "config.h"
#include "cpu.h"
#include "irq_alloc.h"
#include "linker_syms.h"
#include "pic.h"
#include "profile.h"
#include "regdefs.h"
#include "watchdog.h"

IMPLEMENT inline
void
Kernel_thread::free_initcall_section()
{
  // just fill up with invalid opcodes
  for (unsigned short *i = (unsigned short *) &_initcall_start;   
                       i < (unsigned short *) &_initcall_end; i++)
    *i = 0x0b0f;	// UD2 opcode
}

IMPLEMENT FIASCO_INIT
void
Kernel_thread::bootstrap_arch()
{
  // 
  // install our slow trap handler
  //
  nested_trap_handler = base_trap_handler;
  base_trap_handler = thread_handle_trap;

  //
  // initialize interrupts
  //
  Irq_alloc::lookup(2)->alloc(this, false);	// reserve cascade irq  
  Pic::enable(2);			// allow cascaded irqs

  // initialize the profiling timer
  bool user_irq0 = strstr(Boot_info::cmdline(), "irq0");

  if (Config::scheduling_using_pit && user_irq0)
    {
      kdb_ke("option -irq0' not possible since irq 0 is used for scheduling\n"
             "     -- disabling -irq0'");
      user_irq0 = false;
    }

#ifdef CONFIG_PROFILE
  if (Config::profiling)
    {
      if (user_irq0)
        {
          kdb_ke("options -profile' and -irq0' don't mix "
                  "-- disabling -irq0'");
        }
      if (Config::scheduling_using_pit)
        {
          panic("option -profile' not available since PIT is used as "
                "source for timer tick");
        }

      Irq_alloc::lookup(0)->alloc(this, false);

      profile::init();

      if (strstr(Boot_info::cmdline(), " -profstart"))
        profile::start();
    }
  else
#endif
    if (! user_irq0 && ! Config::scheduling_using_pit)
      Irq_alloc::lookup(0)->alloc(this, false); // reserve irq0 even though

  // this has to be done with working scheduling interrupt
  if (Config::apic)
    Apic::init();  

  // this has to be done after Apic::init()
  if (Config::watchdog)
    Watchdog::init();  

  //
  // set PCE-Flag in CR4 to enable read of performace measurement counters
  // in usermode. PMC were introduced in Pentium MMX and PPro processors. 
  //
  if (Cpu::vendor() == Cpu::VENDOR_INTEL &&
     (Cpu::family() == CPU_FAMILY_PENTIUM_PRO || (Cpu::features() & FEAT_MMX)))
    set_cr4 (get_cr4() | CR4_PCE);
}
