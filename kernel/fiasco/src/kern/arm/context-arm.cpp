IMPLEMENTATION[arm]:

#include <cassert>
#include <cstdio>

#include "cpu.h"
#include "globals.h"		// current()
#include "cpu_lock.h"
#include "kmem.h"
#include "lock_guard.h"
#include "space_context.h"
#include "thread_state.h"


IMPLEMENT inline
void Context::switch_fpu( Context *t )
{}

#if 0
/** Switch to a specific different context.  If that context is currently
    locked, switch to its locker instead (except if current() is the locker).
    @pre current() == this  &&  current() != t
    @param t thread that shall be activated.  
    @return false if the context could not be activated, either because it was
            not runnable or because it was not initialized.
 */
IMPLEMENT
Mword Context::switch_to( Context *t )
{
  assert (current() != t);
  assert (current() == this);

  Lock_guard <Cpu_lock> guard (&cpu_lock);

  // Time-slice lending: if t is locked, switch to it's locker
  // instead, this is transitive
  while (t->donatee()                   // target thread not locked
         && t != t->donatee())          // target thread has lock itself
    {
      // Special case for Thread::kill(): If the locker is
      // current(), switch to the locked thread to allow it to
      // release other locks.  Do this only when the target thread
      // actually owns locks.
      if (t->donatee() == current())
        {
          if (t->lock_cnt() > 0)
            break;

          return 0;
        }

      t = t->donatee();
    }

  // Can only switch to running threads!
  if (! (t->state() & Thread_running))  
    return false;

//   // Make sure stack has enough space.  (Irqs must be disabled here
//   // because otherwise we could be preempted between the tests, and
//   // t->kernel_sp could be set to 0 after we checked for that case.)
//   assert(t->kernel_sp == 0 
//       || reinterpret_cast<vm_offset_t>(t->kernel_sp)
//          > reinterpret_cast<vm_offset_t>(t) + sizeof(Context) + 0x20);

  switch_fpu(t);

  if ((state() & Thread_running) && ! in_ready_list())
    ready_enqueue();

  // switch to new thread's stack

  bool ret = 1;

#if 0
  Mword old_sp;
  asm volatile ( "mov %0, sp" : "=r"(old_sp) );
  printf("ASM: switch from %p [sp=%p] to %p [sp=%p]\n",
	 this, (void*)old_sp, t, t->kernel_sp);

  printf("ASM: cont addr = %08x\n", *(t->kernel_sp));
#endif


}
#endif

IMPLEMENT inline
bool Context::switch_cpu( Context *t ) 
{
  //  putchar('+');  
#if 0
  printf("ASM: switch from %p to %p [sp=%p]\n", 
	 this, t, t->kernel_sp);
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
     "   bl call_switchin_context \n" // call switchin_context()   
     
     // return to new context
     "   ldr   pc, [sp], #4       \n"
     "1: ldmia sp!, {fp}       \n"

     : 
     : 
     [new_thread] "r"(t), 
     [old_sp] "r" (&kernel_sp), 
     [new_sp] "r" (t->kernel_sp)
     : "r0", "r4", "r5", "r6", "r7", "r8", "r9", 
     "r10", "r12", "r14", "memory");

  return 1;
}

/** Thread context switchin.  Called on every re-activation of a
    thread (switch_to()).  This method is public only because it is 
    called by an ``extern "C"'' function that is called
    from assembly code (call_switchin_context).
 */
IMPLEMENT 
void Context::switchin_context()
{
  assert(this == current());
  assert(state() & Thread_running);

  // Set kernel-esp in case we want to return to the user.
  // kmem::kernel_esp() returns a pointer to the kernel SP (in the
  // TSS) the CPU uses when next switching from user to kernel mode.  
  // regs() + 1 returns a pointer to the end of our kernel stack.
  Kmem::kernel_sp( reinterpret_cast<Mword*>(regs() + 1) );
#if 0
  printf("switch in address space: %p\n",_space_context);
#endif
  
  // switch to our page directory if nessecary
  _space_context->switchin_context();
}

