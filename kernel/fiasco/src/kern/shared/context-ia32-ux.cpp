IMPLEMENTATION [ia32,amd64,ux]:

#include <cassert>
#include <cstdio>

#include "cpu.h"
#include "globals.h"		// current()
#include "kmem.h"
#include "lock_guard.h"
#include "space.h"
#include "thread_state.h"
#include "kip.h"

IMPLEMENT inline NEEDS ["config.h", "cpu.h"]
void
Context::init_switch_time()
{
  if (Config::fine_grained_cputime)
    _switch_time = Cpu::rdtsc();
}

/**
 * Update consumed CPU time during each context switch and when
 *        reading out the current thread's consumed CPU time.
 */
IMPLEMENT inline NEEDS ["cpu.h"]
void
Context::update_consumed_time()
{
  if (Config::fine_grained_cputime)
    {
      Cpu_time tsc = Cpu::rdtsc();
      consume_time (tsc - _switch_time);
      _switch_time = tsc;
    }
}

PUBLIC inline NEEDS ["kip.h"]
void
Context::update_kip_switch_time(Context * t)
{
  if (Config::fine_grained_cputime)
    {
      Kip::k()->switch_time = _switch_time;
      Kip::k()->thread_time = t->_consumed_time;
    }
}

/** Thread context switchin.  Called on every re-activation of a thread 
    (switch_exec()).  This method is public only because it is called from
    from assembly code in switch_cpu().
 */
IMPLEMENT
void
Context::switchin_context()
{
  assert (this == current());
  assert (state() & Thread_ready);

  // Set kernel-esp in case we want to return to the user.
  // kmem::kernel_sp() returns a pointer to the kernel SP (in the
  // TSS) the CPU uses when next switching from user to kernel mode.  
  // regs() + 1 returns a pointer to the end of our kernel stack.
  *(Kmem::kernel_sp()) = reinterpret_cast<Address>(regs() + 1);

  // load the GDT entries used for TLS
  switch_gdt_tls();

  // switch to our page directory if necessary
  _space->switchin_context();

  // load new segment selectors
  load_segments();

  // update the global UTCB pointer to make the thread find its UTCB 
  // using gs:[0]
  update_utcb_ptr();
}

IMPLEMENT inline NEEDS ["config.h", "cpu.h"]
Cpu_time
Context::consumed_time()
{
  // When using fine-grained CPU time, this is not usecs but TSC ticks
  if (Config::fine_grained_cputime)
    return Cpu::tsc_to_us (_consumed_time);

  return _consumed_time;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [ia32,ux]:

IMPLEMENT inline NEEDS [Context::update_consumed_time,
			Context::update_kip_switch_time,
			Context::store_segments]
void
Context::switch_cpu (Context *t)
{
  Mword dummy1, dummy2, dummy3;

  update_consumed_time();
  update_kip_switch_time(t);

  store_segments();

  asm volatile
    (
     "   pushl %%ebp			\n\t"	// save base ptr of old thread
     "   pushl $1f			\n\t"	// restart addr to old stack
     "   movl  %%esp, (%0)		\n\t"	// save stack pointer
     "   movl  (%1), %%esp		\n\t"	// load new stack pointer
     						// in new context now (cli'd)
     "   movl  %2, %%eax		\n\t"	// new thread's "this"
     "   call  switchin_context_label	\n\t"	// switch pagetable
     "   popl  %%eax			\n\t"	// don't do ret here -- we want
     "   jmp   *%%eax			\n\t"	// to preserve the return stack
						// restart code
     "  .p2align 4			\n\t"	// start code at new cache line
     "1: popl %%ebp			\n\t"	// restore base ptr

     : "=c" (dummy1), "=S" (dummy2), "=D" (dummy3)
     : "c" (&_kernel_sp), "S" (&t->_kernel_sp), "D" (t)
     : "eax", "ebx", "edx", "memory");
}

//---------------------------------------------------------------------------
IMPLEMENTATION [amd64]:

IMPLEMENT inline NEEDS [Context::update_consumed_time,
			Context::update_kip_switch_time,
			Context::store_segments]
void
Context::switch_cpu (Context *t)
{
  Mword dummy1, dummy2, dummy3;

  update_consumed_time();
  update_kip_switch_time(t);

  store_segments();

  asm volatile
    (
     "   push %%rbp			\n\t"	// save base ptr of old thread
     "   pushq $1f			\n\t"	// push restart addr on old stack
     "   mov  %%rsp, (%0)		\n\t"	// save stack pointer
     "   mov  (%1), %%rsp		\n\t"	// load new stack pointer
     // in new context now (cli'd)
     "   pushq %2			\n\t"	// new thread's "this"
     "   call  switchin_context_label	\n\t"	// switch pagetable
     "   add  $8, %%rsp			\n\t"	// clean up args
     "   pop  %%rax			\n\t"	// don't do ret here -- we want
     "   jmp   *%%rax			\n\t"	// to preserve the return stack
     // restart code
     "  .p2align 4			\n\t"	// start code at new cache line
     "1: pop %%rbp			\n\t"	// restore base ptr

     : "=c" (dummy1), "=S" (dummy2), "=D" (dummy3)
     : "c" (&_kernel_sp), "S" (&t->_kernel_sp), "D" (t)
     : "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "rax", "rbx", "rdx", "memory");
}

//---------------------------------------------------------------------------
IMPLEMENTATION [{ia32,amd64,ux}-!utcb]:

inline
void
Context::update_utcb_ptr() 
{}


//---------------------------------------------------------------------------
IMPLEMENTATION [{ia32,ux,amd64}-utcb]:

/**
 * Update current UTCB Pointer and sets the FPU used Bit in the UTCB
 * if necessary. LIPC is only allowed with FPU disabled
 */
PROTECTED inline NEEDS ["globals.h"]
void
Context::update_utcb_ptr() 
{
  *global_utcb_ptr = local_id();

  /* An optimization here would be the test if the fpu is disabled */
  //  if(state() % Thread_fpu_owner)
  //    utcb()->set_fpu_bit_dirty();
  //  else
  //    utcb()->clear_fpu_bit_dirty();
}


//---------------------------------------------------------------------------
IMPLEMENTATION [!{ia32-segments}]:

PROTECTED inline
void
Context::switch_gdt_tls()
{}


//---------------------------------------------------------------------------
IMPLEMENTATION [segments]:

PROTECTED inline NEEDS["cpu.h"]
void
Context::load_segments()
{
  Cpu::set_es((Unsigned32)_es);
  Cpu::set_fs((Unsigned32)_fs);
  Cpu::set_gs((Unsigned32)_gs);
}

PROTECTED inline NEEDS["cpu.h"]
void
Context::store_segments()
{
  _es = Cpu::get_es();
  _fs = Cpu::get_fs();
  _gs = Cpu::get_gs();
}


//---------------------------------------------------------------------------
IMPLEMENTATION [!segments]:

PROTECTED inline
void
Context::load_segments()
{}

PROTECTED inline
void
Context::store_segments()
{}
