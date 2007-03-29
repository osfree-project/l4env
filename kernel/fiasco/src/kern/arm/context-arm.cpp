IMPLEMENTATION [arm]:

#include <cassert>
#include <cstdio>

#include "globals.h"		// current()
#include "l4_types.h"
#include "cpu_lock.h"
#include "kmem.h"
#include "lock_guard.h"
#include "space.h"
#include "thread_state.h"

IMPLEMENT inline
Mword
Context::state() const
{
  register Mword res asm("r14");
  asm volatile ("ldr %0, [%0]   \n"
		: "=r" (res) : "0" (&_state));
  return res;
}

IMPLEMENT inline
Mword
Context::is_tcb_mapped() const
{
  register Mword pagefault_if_0 asm("r14");
  asm volatile ("msr cpsr_f, #0 \n" // clear flags
                "ldr %0, [%0]   \n"
		"movne %0, #1   \n"
		"moveq %0, #0   \n"
		: "=r" (pagefault_if_0) : "0" (&_state));
  return pagefault_if_0;
}

IMPLEMENT inline
void
Context::init_switch_time()
{}

IMPLEMENT inline
void
Context::switch_fpu(Context *)
{}

IMPLEMENT inline
void
Context::update_consumed_time()
{}

IMPLEMENT inline
void
Context::switch_cpu(Context *t) 
{
  unsigned long dummy1, dummy2, dummy3;
  //  putchar('+');  
#if 0
  printf("ASM: switch from %p to %p [sp=%p]\n", 
	 this, t, t->_kernel_sp);
#endif
  asm volatile
    (// save context of old thread
     "   stmdb sp!, {fp}       \n"
     "   adr   lr, 1f             \n"
     "   str   lr, [sp, #-4]!     \n"
     "   str   sp, [%[old_sp]]    \n"

     // switch to new stack
     "   mov   sp, %[new_sp]      \n"
     "   mov   r0, %[new_thread]  \n"

     // deliver requests to new thread
     "   bl switchin_context_label \n" // call Context::switchin_context()   
     
     // return to new context
     "   ldr   pc, [sp], #4       \n"
     "1: ldmia sp!, {fp}       \n"

     :
     [new_thread] "=r"(dummy1),
     [old_sp]     "=r"(dummy2),
     [new_sp]     "=r"(dummy3)
     : 
     "0"(t), 
     "1" (&_kernel_sp), 
     "2" (t->_kernel_sp)
     : "r0", "r4", "r5", "r6", "r7", "r8", "r9", 
     "r10", "r12", "r14", "memory");
}

/** Thread context switchin.  Called on every re-activation of a
    thread (switch_exec()).  This method is public only because it is 
    called by an ``extern "C"'' function that is called
    from assembly code (call_switchin_context).
 */
IMPLEMENT 
void Context::switchin_context()
{
  assert(this == current());
  assert(state() & Thread_ready);

  // Set kernel-esp in case we want to return to the user.
  // kmem::kernel_esp() returns a pointer to the kernel SP (in the
  // TSS) the CPU uses when next switching from user to kernel mode.  
  // regs() + 1 returns a pointer to the end of our kernel stack.
  Kmem::kernel_sp( reinterpret_cast<Mword*>(regs() + 1) );
#if 0
  printf("switch in address space: %p\n",_space);
#endif
  
  // switch to our page directory if nessecary
  _space->switchin_context();
}

IMPLEMENT inline
Cpu_time
Context::consumed_time()
{
  return _consumed_time;
}
