IMPLEMENTATION[ia32-ux]:

#include <flux/x86/seg.h>

#include <cassert>
#include <cstdio>

#include "cpu.h"
#include "globals.h"		// current()
#include "cpu_lock.h"
#include "kmem.h"
#include "lock_guard.h"
#include "space_context.h"
#include "thread_state.h"
#include "logdefs.h"

/** Switch to a specific different context.  If that context is currently
    locked, switch to its locker instead (except if current() is the locker).
    @pre current() == this  &&  current() != t
    @param t thread that shall be activated.  
    @return false if the context could not be activated, either because it was
            not runnable or because it was not initialized.
 */
IMPLEMENT
Mword Context::switch_to(Context *t)
{
  assert (current() != t);
  assert (current() == this);

  Lock_guard <Cpu_lock> guard (&cpu_lock);

  // Time-slice lending: if t is locked, switch to it's locker
  // instead, this is transitive
  while (t->donatee()                   // target thread not locked
         && t != t->donatee())          // target thread has lock itself
    {
      // Special case for thread_t::kill(): If the locker is
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

  LOG_CONTEXT_SWITCH;

  // switch to new thread's stack

  bool ret;
  unsigned dummy1, dummy2, dummy3;

  if (Config::fine_grained_cputime)
    {
      // switch will fail on t->kernel_sp == 0
      if (t->kernel_sp)
        {
          unsigned long tsc_32;

          asm volatile ("rdtsc" : "=a" (tsc_32) : : "edx");
          sched()->stop_thread (tsc_32);
          t->sched()->start_thread (tsc_32);
        }
    }
  asm volatile
    (// save context of old thread
     "   pushl %%ebp \n"
     "   pushl $1f \n"          // push restart address on old stack
     "   movl  %%esp, (%1) \n"  // save stack pointer

     // read context of new thread
     "   movl  (%2), %%esp \n"  // load new stack pointer - now in other thread
     "   testl %%esp,%%esp \n"  // check new stack pointer - is it 0?
     "   jz    3f \n"           // yes - fail

     // in new context now (still cli'd)

     // deliver requests to new thread
     "   pushl %3 \n"           // new thread's "this" -- XXX correct?
     "   call  call_switchin_context \n" // call switchin_context()   
     "   addl  $4,%%esp \n"     // clean up args

     // return to old context
     "   ret \n"                // jump to restart address on stack

     // code for failure
     "3: \n"
     "   movl  (%1), %%esp \n"  // load old stack pointer
     "   addl  $4, %%esp \n"
     "   xor   %0,%0 \n"        // return false
     "   jmp   2f \n"

     // label for our restart address
     "   .p2align 4 \n"         // start code at new cache line
     "1: mov   $1,%0 \n"        // return true
     "2: \n"                    // label for end of this asm segment
     "   popl %%ebp \n"
     : "=a" (ret), "=c" (dummy1), "=S" (dummy2), "=D" (dummy3)
     : "1" (&kernel_sp), "2" (&t->kernel_sp), "3" (t)         
     : "ebx", "edx", "memory");

  return ret;
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
  *(Kmem::kernel_esp()) = reinterpret_cast<vm_offset_t>(regs() + 1);
  
  // switch to our page directory if nessecary
  _space_context->switchin_context();
}

