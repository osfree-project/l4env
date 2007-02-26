INTERFACE [ux]:

#include <sys/types.h>

class Trap_state;

EXTENSION class Thread
{
private:
  static int (*int3_handler)(Trap_state*);
};

IMPLEMENTATION [ux]:

#include <cstdio>
#include <csignal>
#include "boot_info.h"
#include "config.h"		// for page sizes
#include "emulation.h"
#include "kdb_ke.h"
#include "lock_guard.h"
#include "panic.h"
#include "utcb_init.h"

int      (*Thread::int3_handler)(Trap_state*);

IMPLEMENT static inline NEEDS ["emulation.h"]
Mword
Thread::exception_cs()
{
  return Emulation::kernel_cs();
}

/**
 * The ux specific part of the thread constructor.
 */
PRIVATE inline
void
Thread::arch_init()
{
  // Allocate FPU state now because it indirectly calls current()
  // save_state runs on a signal stack and current() doesn't work there.
  Fpu_alloc::alloc_state(fpu_state());
  
  // clear out user regs that can be returned from the thread_ex_regs
  // system call to prevent covert channel
  Entry_frame *r = regs();
  r->sp(0);
  r->ip(0);
  r->cs(Emulation::kernel_cs() & ~1);			// force iret trap
  r->ss(Emulation::kernel_ss());

  if(Config::enable_io_protection)
    r->flags(EFLAGS_IOPL_K | EFLAGS_IF | 2);
  else
    r->flags(EFLAGS_IOPL_U | EFLAGS_IF | 2);		// XXX iopl=kernel
}

PUBLIC static inline
void
Thread::set_int3_handler(int (*handler)(Trap_state *ts))
{
  int3_handler = handler;
}

PRIVATE inline bool Thread::check_trap13_kernel (Trap_state *, bool)
{ return 1; }

PRIVATE inline void Thread::check_f00f_bug (Trap_state *)
{}

PRIVATE inline int  Thread::handle_io_page_fault (Trap_state *, Address, bool)
{ return 0; }

PRIVATE inline bool Thread::handle_sysenter_trap (Trap_state *, Address, bool)
{ return true; }

PRIVATE inline bool Thread::trap_is_privileged (Trap_state *)
{ return true; }

PRIVATE inline bool Thread::handle_smas_gp_fault ()
{ return false; }

PRIVATE inline
void
Thread::do_wrmsr_in_kernel (Trap_state *)
{
  // do "wrmsr (msr[ecx], edx:eax)" in kernel
  kdb_ke("wrmsr not supported");
}

PRIVATE inline
void
Thread::do_rdmsr_in_kernel (Trap_state *)
{
  // do "rdmsr (msr[ecx], edx:eax)" in kernel
  kdb_ke("rdmsr not supported");
}

PRIVATE inline
int
Thread::handle_not_nested_trap (Trap_state *)
{ return -1; }

PRIVATE
int
Thread::call_nested_trap_handler (Trap_state *ts)
{
  extern int gdb_trap_recover;

  // run the nested trap handler on a separate stack
  // equiv of: return nested_trap_handler(ts) == 0 ? true : false;

  int ret;
  static char nested_handler_stack [Config::PAGE_SIZE];

  unsigned dummy1, dummy2;
  asm volatile
    ("movl   %%esp,%%edx	\n\t"
     "orl    %%ebx,%%ebx	\n\t"    // don't set %esp if gdb fault recovery
     "jz     1f			\n\t"
     "movl   %%ebx,%%esp	\n\t"
     "1:			\n\t"
     "pushl  %%edx		\n\t"
     "call   *%5		\n\t"
     "popl   %%esp		\n\t"
     : "=a" (ret), "=c" (dummy1), "=d" (dummy2)
     : "a"(ts), "b" (gdb_trap_recover 
		? 0 : (nested_handler_stack + sizeof(nested_handler_stack))),
       "m" (nested_trap_handler)
     : "memory");

  assert (_magic == magic);

  // Do shutdown by switching to idle loop, unless we're already there
  if (!running && current() != kernel_thread)
    current()->switch_to_locked (kernel_thread);

  return ret == 0 ? 0 : -1;
}

// The "FPU not available" trap entry point
extern "C" void thread_handle_fputrap (void) { panic ("fpu trap"); }

extern "C" void thread_timer_interrupt_stop() {}
extern "C" void thread_timer_interrupt_slow() {}

IMPLEMENT
void
Thread::user_invoke()
{
  Mword dummy;

  Cpu::set_gs(Utcb_init::gs_value());

  asm volatile
    ("  movl %%ds ,  %0         \n\t"
     "  movl %0,     %%es	\n\t"
     : "=r"(dummy));

  user_invoke_generic();

  asm volatile
    ("	cmpl $2    , %%edx      \n\t"     // Sigma0
     "	je   1f                 \n\t"
     "  cmpl $4    , %%edx      \n\t"     // Roottask
     "  je   1f                 \n\t"
     "  xorl %%ebx , %%ebx      \n\t"     // don't pass info if other
     "  xorl %%ecx , %%ecx      \n\t"     // Sigma0 or Roottask
     "1:                        \n\t"
     "  movl %%eax , %%esp      \n\t"
     "  xorl %%edx , %%edx      \n\t"
     "  xorl %%esi , %%esi      \n\t"     // clean out user regs      
     "  xorl %%edi , %%edi      \n\t"                           
     "  xorl %%ebp , %%ebp      \n\t"
     "  xorl %%eax , %%eax      \n\t"
     "  iret                    \n\t"
     :                          	// no output
     : "a" (nonull_static_cast<Return_frame*>(current()->regs())),
       "b" (Kmem::virt_to_phys(Boot_info::mbi_virt())),
       "c" (Kmem::virt_to_phys(Kip::k())),
       "d" ((unsigned) Thread::lookup(current())->id().task()));
}
//---------------------------------------------------------------------------
IMPLEMENTATION [ux-segments]:

#include <feature.h>
KIP_KERNEL_FEATURE("segments");

#include "gdt.h"

PRIVATE inline NEEDS["gdt.h"]
bool
Thread::handle_lldt(Trap_state *ts)
{
  Address desc_addr;
  int size;
  unsigned int entry_number, task;
  Mem_space *s;
  Thread *t;

  if (EXPECT_TRUE
      (ts->ip() >= Kmem::mem_user_max - 4 ||
       (mem_space()->peek_user((Mword*)ts->ip()) & 0xffffff) != 0xd0000f))
    return 0;

#ifndef GDT_ENTRY_TLS_MIN
#define GDT_ENTRY_TLS_MIN 6
#endif

  if (EXPECT_FALSE(ts->_ebx == 0)) // size argument
    {
      ts->_ebx  = GDT_ENTRY_TLS_MIN;
      ts->ip(ts->ip() + 3);
      return 1;
    }

  if (0 == tls_setup_emu(ts, &desc_addr, &size, &entry_number, &t))
    {
      Mword *trampoline_page = (Mword *) Kmem::phys_to_virt
			       (Mem_layout::Trampoline_frame);

      while (size >= Cpu::Ldt_entry_size)
        {
          Gdt_entry desc(mem_space()->peek_user((Unsigned64 *)desc_addr));

	  if (desc.limit())
	    {
              Ldt_user_desc info;

	      info.entry_number    = entry_number + GDT_ENTRY_TLS_MIN;
	      info.base_addr       =  desc.base();
	      info.limit           =  desc.limit();
	      info.seg_32bit       =  desc.seg32();
	      info.contents        =  desc.contents();
	      info.read_exec_only  = !desc.writable();
	      info.limit_in_pages  =  desc.granularity();
	      info.seg_not_present = !desc.present();
	      info.useable         =  desc.avl();

	      // Set up data on trampoline
	      for (unsigned i = 0; i < sizeof(info) / sizeof(Mword); i++)
		*(trampoline_page + i + 1) = *(((Mword *)&info) + i);

	      s = Space_index(t->id().task()).lookup()->mem_space();

	      // Call set_thread_area for given user process
	      Trampoline::syscall(s->pid(), 243 /* __NR_set_thread_area */,
				  Mem_layout::Trampoline_page + sizeof(Mword));

	      // Also set this for the fiasco kernel so that
	      // segment registers can be set, this is necessary for signal
	      // handling, esp. for sigreturn to work in the Fiasco kernel
	      // with the context of the client (gs/fs values).
	      Emulation::thread_area_host(entry_number + GDT_ENTRY_TLS_MIN);
	    }

	  size      -= Cpu::Ldt_entry_size;
	  desc_addr += Cpu::Ldt_entry_size;
	  entry_number++;
	}

      return 1;
    }

  if (lldt_setup_emu(ts, &s, &desc_addr, &size, &entry_number, &task))
    return 1;

  Mword *trampoline_page = (Mword *) Kmem::phys_to_virt
			   (Mem_layout::Trampoline_frame);

  while (size >= Cpu::Ldt_entry_size)
    {
      Ldt_user_desc info;

      Gdt_entry desc(mem_space()->peek_user((Unsigned64 *)desc_addr));

      info.entry_number    = entry_number;
      info.base_addr       =  desc.base();
      info.limit           =  desc.limit();
      info.seg_32bit       =  desc.seg32();
      info.contents        =  desc.contents();
      info.read_exec_only  = !desc.writable();
      info.limit_in_pages  =  desc.granularity();
      info.seg_not_present = !desc.present();
      info.useable         =  desc.avl();

      // Set up data on trampoline
      for (unsigned i = 0; i < sizeof(info) / sizeof(Mword); i++)
	*(trampoline_page + i + 1) = *(((Mword *)&info) + i);

      // Call modify_ldt for given user process
      Trampoline::syscall(s->pid(), __NR_modify_ldt,
			  1, // write LDT
			  Mem_layout::Trampoline_page + sizeof(Mword),
			  sizeof(info));

      // Also set this for the fiasco kernel so that
      // segment registers can be set, this is necessary for signal
      // handling, esp. for sigreturn to work in the Fiasco kernel
      // with the context of the client (gs/fs values).
      if (handle_lldt_overwrite_local_ldt(*(trampoline_page + 1)))
	Emulation::modify_ldt(*(trampoline_page + 1), // entry
			      0,                      // base
			      1);                     // size

      size      -= Cpu::Ldt_entry_size;
      desc_addr += Cpu::Ldt_entry_size;
      entry_number++;
    }

  return 1;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [ux-segments-utcb]:

/** The UTCB already has entry 0, so do not overwrite it.
 */
PRIVATE inline
bool
Thread::handle_lldt_overwrite_local_ldt(Mword nr)
{ return nr; }

//---------------------------------------------------------------------------
IMPLEMENTATION [ux-segments-!utcb]:

/** All entries available, always return ok.
 */
PRIVATE inline
bool
Thread::handle_lldt_overwrite_local_ldt(Mword)
{ return 1; }

//---------------------------------------------------------------------------
IMPLEMENTATION [ux-!segments]:

PRIVATE inline
bool
Thread::handle_lldt(Trap_state *)
{ return 0; }
