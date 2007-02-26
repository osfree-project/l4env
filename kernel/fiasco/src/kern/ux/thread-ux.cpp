INTERFACE:

#include <sys/types.h>

class trap_state;

IMPLEMENTATION[ux]:

#include <cstdio>
#include <csignal>
#include "boot_info.h"		// for boot_info
#include "config.h"		// for page sizes
#include "cpu.h"		// for cpu
#include "cpu_lock.h"
#include "emulation.h"
#include "fpu_alloc.h"
#include "kdb_ke.h"		// for kdb_ke
#include "lock_guard.h"
#include "panic.h"
#include "processor.h"		// for cli/sti
#include "regdefs.h"
#include "sched_context.h"
#include "vmem_alloc.h"

/** The global trap handler switch.
    This function handles CPU-exception reflection, emulation of CPU 
    instructions (LIDT, WRMSR, RDMSR), int3 debug messages, 
    kernel-debugger invocation, and thread crashes (if a trap cannot be 
    handled).
    @param state trap state
    @return 0 if trap has been consumed by handler;
           -1 if trap could not be handled.
 */    
PUBLIC 
int
Thread::handle_slow_trap (trap_state *ts)
{
  extern unsigned gdb_trap_recover; // in OSKit's gdb_trap.c
  Address eip;

  if (EXPECT_FALSE (gdb_trap_recover))
    goto generic_debug; // we're in the GDB stub -- let generic handler handle it
  
  LOG_TRAP;

  if (EXPECT_FALSE (!((ts->cs & 3) || (ts->eflags & EFLAGS_VM))))
    goto generic_debug;		// we were in kernel mode -- nothing to emulate

  if (EXPECT_FALSE (ts->trapno == 2))	// NMI?
    goto generic_debug;			// NMI always enters kernel debugger

  if (EXPECT_FALSE (ts->trapno == 0xffffffff))		// debugger interrupt
    goto generic_debug;         

  // so we were in user mode -- look for something to emulate

  // We continue running with interrupts off -- no sti() here.

  // Set up exception handling.  If we suffer an un-handled user-space
  // page fault, kill the thread.
  jmp_buf pf_recovery;    
  unsigned error;         
  if (EXPECT_FALSE ((error = setjmp(pf_recovery)) != 0))
    {
      printf ("KERNEL: %x.%x (tcb=0x%x) killed: unhandled page fault, "
	      "code=0x%x\n",
	      debug_taskno(), debug_threadno(),
	      (unsigned) this, error);
      goto fail_nomsg;    
    }

  _recover_jmpbuf = &pf_recovery;

  eip = ts->eip;   

  // check for "invalid opcode" exception
  if ((Config::ABI_V4 || Config::X2_LIKE_SYS_CALLS) && ts->trapno == 6)
    {
      if (peek_user ((Unsigned16 *) ts->eip) == 0x90f0)	// "lock; nop" opcode
	{
	  ts->eip += 2;			// step behind the opcode
	  ts->eax = space()->kip_address();
	  goto success;
	}
    }

  // check for "general protection" exception
  if (ts->trapno == 13)   
    {
      // check for "lidt (%eax)"
      if ((ts->err & 0xffff) == 0 && eip < Kmem::mem_user_max - 4 &&
          (peek_user((Mword*)eip) & 0xffffff) == 0x18010f) {

        // read descriptor
        if (ts->eax >= Kmem::mem_user_max - 6)
          goto fail;    

        x86_gate *idt = peek_user ((x86_gate **)(ts->eax + 2));
        vm_size_t limit = Config::backward_compatibility ?
                          255 : peek_user ((Unsigned16 *) ts->eax);

        if (limit >= Kmem::mem_user_max ||
            reinterpret_cast<Address>(idt) >= Kmem::mem_user_max - limit - 1)
          goto fail;

        // OK; store descriptor
        _idt = idt;     
        _idt_limit = (limit + 1) / sizeof(x86_gate);

        ts->eip += 3;		          // consume instruction and continue
        goto success;
      }

      // check for "wrmsr (%eax)"
      if ((ts->eflags & EFLAGS_IOPL) == EFLAGS_IOPL_U // only allow priv tasks
          && (ts->err & 0xffff) == 0
          && eip < Kmem::mem_user_max - 2
          && (peek_user((Unsigned16*)eip) ) == 0x300f
          && (Cpu::features() & FEAT_MSR)       // cpu has MSRs
         )
        {
          // do "wrmsr (msr[ecx], edx:eax)" in kernel
          unsigned dummy1, dummy2, dummy3;

          asm volatile(".byte 0xf; .byte 0x30\n"        /* wrmsr */
                      : "=a" (dummy1), "=d" (dummy2), "=c" (dummy3)
                      : "0" (ts->eax), "1" (ts->edx), "2" (ts->ecx));

          // consume instruction and continue
          smp_cas(&ts->eip, eip, eip + 2); // ignore errors
          goto success;   
        }

      // check for "rdmsr (%eax)"
      if ((ts->eflags & EFLAGS_IOPL) == EFLAGS_IOPL_U // only allow priv tasks
          && (ts->err & 0xffff) == 0
          && eip < Kmem::mem_user_max - 2
          && (peek_user((Unsigned16*)eip) ) == 0x320f
          && (Cpu::features() & FEAT_MSR)       // cpu has MSRs
         )
        {
          // do "rdmsr (msr[ecx], edx:eax)" in kernel
          unsigned dummy; 

          asm volatile(".byte 0xf; .byte 0x32\n"        /* rdmsr */
                       : "=a" (ts->eax), "=d" (ts->edx), "=c" (dummy)
                       : "2" (ts->ecx));

          // consume instruction and continue
          smp_cas(&ts->eip, eip, eip + 2); // ignore errors
          goto success;   
        }

      // check for "hlt" -> deliver L4 version
      if ((ts->eflags & EFLAGS_IOPL) == EFLAGS_IOPL_U // only allow priv tasks
          && (ts->err & 0xffff) == 0
          && eip < Kmem::mem_user_max - 1
          && peek_user ((Unsigned8 *) eip) == 0xf
         )
        {
          // FIXME: What version should Fiasco deliver?
          ts->eax = 0x00010000;

          // consume instruction and continue
          smp_cas(&ts->eip, eip, eip + 1); // ignore errors
          goto success;   
        }
    }

  // let's see if we have a trampoline to invoke
  if (ts->trapno < 0x20 && ts->trapno < _idt_limit)
    {
      // translate page fault exceptions to general protection
      // exception -- users shouldn't have to care about about page
      // faults (and probably couldn't find out the fault address
      // anyway as they can't access the %cr2 (page fault address)
      // register)        
      if (ts->trapno == 14)
        {
          ts->trapno = 13;
          ts->err = 0;    
        }

      x86_gate g = peek_user (_idt + ts->trapno);

      if (Config::backward_compatibility // backward compat.: don't check
          || ((g.word_count & 0xe0) == 0
              && (g.access & 0x1f) == 0x0f)) // gate descriptor ok?
        {
          Address handler = (g.offset_high << 16) | g.offset_low;

          if ((handler != 0 || !Config::backward_compatibility) // bw compat.: != 0?
              && handler < Kmem::mem_user_max // in user space?
              && ts->esp <= Kmem::mem_user_max
              && ts->esp > 4 * 4) // enough space on user stack?
            {
              // OK, reflect the trap to user mode
              Unsigned32 esp = ts->esp;

              if (! raise_exception(ts, handler))
                {         
                  // someone interfered and changed our state
                  check(state_del(Thread_cancel));

                  goto success;
                }

              esp -= sizeof (Mword);
              poke_user ((Mword*) esp, ts->eflags);
	      esp -= sizeof (Mword);
              poke_user ((Mword*) esp, (Mword) Emulation::get_kernel_cs());
	      esp -= sizeof (Mword);
              poke_user ((Mword*) esp, eip);

              /* reset single trap flag to mirror cpu correctly */
              if (ts->trapno == 1)
                ts->eflags &= ~EFLAGS_TF;

              // need to push error code?
              if ((ts->trapno >= 0x0a && ts->trapno <= 0x0e) ||
                   ts->trapno == 0x08 ||
                   ts->trapno == 0x11) 
                {
	          esp -= sizeof (Mword);
                  poke_user ((Mword*) esp, ts->err);
                }

              goto success;		// we've consumed the trap
            }
        }
    }

  // backward compatibility cruft: check for those insane "int3" debug
  // messaging command sequences
  if (ts->trapno == 3)
    goto generic_debug;

  // privileged tasks also may invoke the kernel debugger with a debug
  // exception
  if (ts->trapno == 1)
    goto generic_debug;       

  // can't handle trap -- kill the thread

fail:
  printf ("KERNEL: %x.%x (tcb=%08x) killed:\n"
         "\033[1mUnhandled trap\033[m\n",
	  debug_taskno(), debug_threadno(),
	  (unsigned) this);

fail_nomsg:
  trap_dump (ts);

  if (Config::conservative)
    kdb_ke ("thread killed");

  // Cancel must be cleared on all kernel entry paths. See slowtraps for
  // why we delay doing it until here.
  state_del (Thread_cancel);

  // we haven't been re-initialized (cancel was not set) -- so sleep
  if (state_change_safely (~Thread_running, Thread_cancel | Thread_dead))
    while (! (state() & Thread_running))
      schedule();       

success:
  _recover_jmpbuf = 0;
  return 0;

generic_debug:
  _recover_jmpbuf = 0;    

  if (!nested_trap_handler)
    return -1;

  // run the nested trap handler on a separate stack
  // equiv of: return nested_trap_handler(ts) == 0 ? true : false;

  int ret;
  static char nested_handler_stack[Config::PAGE_SIZE];

  unsigned dummy1, dummy2;
  asm volatile
    (" movl %%esp,%%eax \n"
     " orl  %%ebx,%%ebx \n"     // don't set %esp if gdb fault recovery
     " jz   1f \n"        
     " movl %%ebx,%%esp \n"
     "1: pushl %%eax \n"  
     " pushl %%ecx \n"    
     " call *%%edx \n"    
     " addl $4,%%esp \n"  
     " popl %%esp"        
     : "=a" (ret), "=c" (dummy1), "=d" (dummy2)
     : "b" (gdb_trap_recover ?  // gdb fault recovery?
            0 : (nested_handler_stack + sizeof(nested_handler_stack))),
       "1" (ts),          
       "2" (nested_trap_handler)
     : "memory");         

  assert (_magic == magic);

  // Do shutdown by switching to idle loop, unless we're already there
  if (!running && current() != kernel_thread)
    current()->switch_to (kernel_thread);

  return ret == 0 ? 0 : -1;
}

PUBLIC inline
int
Thread::is_mapped()
{
  return 1;
}

// The "FPU not available" trap entry point
extern "C" void
thread_handle_fputrap(void)
{
  panic ("fpu trap");
}

inline void
thread_timer_interrupt_arch()
{
  LOG_TIMER_IRQ(0);
}

extern "C" void
thread_timer_interrupt_stop()
{}

extern "C" void
thread_timer_interrupt_slow()
{}
