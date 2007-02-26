/*
 * Fiasco ia32 / UX
 * Shared main startup/shutdown code
 */

INTERFACE:

#include "initcalls.h"

IMPLEMENTATION[ia32-ux]:

#include <cstdio>
#include "config.h"
#include "cpu.h"
#include "globals.h"
#include "kernel_thread.h"
#include "kmem.h"
#include "linker_syms.h"
#include "processor.h"

FIASCO_INIT int main (void) {

  unsigned dummy;

  // caution: no stack variables in this function because we're going
  // to change the stack pointer!

  printf ("%s\n", reinterpret_cast<char*>(Kmem::info()) +
         (Kmem::info()->offset_version_strings << 4));

  printf ("CPU: %s (%u.%u.%u) Model: %s at %llu MHz\n\n",
           Cpu::vendor_str(), Cpu::family(), Cpu::model(),
           Cpu::stepping(), Cpu::model_str(),
           Cpu::frequency() / 1000000);

  if (Cpu::inst_tlb_entries())
    printf ("%4u Entry I TLB (4K pages)\n", Cpu::inst_tlb_entries());
  if (Cpu::data_tlb_entries())
    printf ("%4u Entry D TLB (4K pages)\n", Cpu::data_tlb_entries());

  if (Cpu::l1_trace_cache_size())
    printf ("%3uK %c-ops T Cache (%u-way associative)\n",
             Cpu::l1_trace_cache_size(),
#ifdef CONFIG_UX
	     'µ',
#else
	     '\346',
#endif
             Cpu::l1_trace_cache_asso());

  else if (Cpu::l1_inst_cache_size())
    printf ("%4u KB L1 I Cache (%u-way associative, %u bytes per line)\n",
             Cpu::l1_inst_cache_size(),
             Cpu::l1_inst_cache_asso(),
             Cpu::l1_inst_cache_line_size());

  if (Cpu::l1_data_cache_size())
    printf ("%4u KB L1 D Cache (%u-way associative, %u bytes per line)\n"
            "%4u KB L2 U Cache (%u-way associative, %u bytes per line)\n\n",
             Cpu::l1_data_cache_size(),
             Cpu::l1_data_cache_asso(),
             Cpu::l1_data_cache_line_size(),
             Cpu::l2_cache_size(),
             Cpu::l2_cache_asso(),
             Cpu::l2_cache_line_size());

  printf ("Freeing init code/data: %u bytes (%u pages)\n\n",
          &_initcall_end - &_initcall_start,
          &_initcall_end - &_initcall_start >> Config::PAGE_SHIFT);

  // Perform architecture specific initialization
  main_arch();

  // all initializations done, go start the system
  Proc::sti();

  // create kernel thread
  static Kernel_thread *kernel = new (&Config::kernel_id) Kernel_thread;
  nil_thread = kernel_thread = kernel; // fill public variable

  // switch to stack of kernel thread and bootstrap the kernel
  asm volatile
    ("	movl %%esp, %0		\n\t"	// save stack pointer in safe variable
     "	movl %4, %%esp		\n\t"	// switch stack
     "	pushl %5		\n\t"	// push "this" pointer
     "	call call_bootstrap	\n\t"	// bootstrap kernel thread
     :	"=m" (boot_stack), "=a" (dummy), "=c" (dummy), "=d" (dummy)
     :	"S" (kernel->init_stack()), "D" (kernel));
}
