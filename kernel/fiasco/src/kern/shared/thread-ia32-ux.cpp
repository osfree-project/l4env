/*
 * Fiasco Thread Code
 * Shared between UX and native IA32.
 */
INTERFACE [ia32,ux]:

#include "trap_state.h"

class Idt_entry;

EXTENSION class Thread 
{
private:
  /**
   * Return code segment used for exception reflection to user mode
   */
  static Mword const exception_cs();

protected:
  Idt_entry *_idt;
  Unsigned16 _idt_limit;

  static Trap_state::Handler nested_trap_handler FIASCO_FASTCALL;
};

//----------------------------------------------------------------------------
IMPLEMENTATION [ia32,ux]:

#include "config.h"
#include "cpu.h"
#include "cpu_lock.h"
#include "mem_layout.h"
#include "logdefs.h"
#include "paging.h"
#include "processor.h"		// for cli/sti
#include "regdefs.h"
#include "std_macros.h"
#include "thread.h"
#include "timer.h"
#include "trap_state.h"
#include "vmem_alloc.h"

#ifdef CONFIG_KDB
extern unsigned gdb_trap_recover; // in gdb_trap.c
#else
unsigned gdb_trap_recover;
#endif

Trap_state::Handler Thread::nested_trap_handler FIASCO_FASTCALL;

IMPLEMENT
Thread::Thread (Task* task, L4_uid id,
                unsigned short init_prio, unsigned short mcp)
      : Receiver (&_thread_lock,
                  task,
                  init_prio,
                  mcp,
                  Config::default_time_slice),
        Sender            (id, 0),   // select optimized version of constructor
        _preemption       (id),
        _deadline_timeout (&_preemption),
	_activation	  (id)
{
  assert (current() == thread_lock()->lock_owner());
  assert (state() == Thread_invalid);

  if (Config::stack_depth)
    std::memset((char*)this + sizeof(Thread), '5',
		Config::thread_block_size-sizeof(Thread)-64);

  _magic          = magic;
  _task           = task;
  _irq            = 0;
  _idt_limit      = 0;
  _recover_jmpbuf = 0;
  _timeout        = 0;

  *reinterpret_cast<void(**)()> (--_kernel_sp) = user_invoke;

  arch_init();

  setup_utcb_kernel_addr();

  _pager = _ext_preempter = nil_thread;

  preemption()->set_receiver (nil_thread);

  if (space_index() == Thread::lookup(thread_lock()->lock_owner())
      ->space_index())
    {
      // same task -> enqueue after creator
      present_enqueue (Thread::lookup(thread_lock()->lock_owner()));
    }
  else
    enqueue_thread_other_task();

  state_add(Thread_dead);
  
  // ok, we're ready to go!
}

/** (Re-) Ininialize a thread and make it ready.
    XXX Contrary to the L4-V2 spec we only cancel IPC if eip != 0xffffffff!
    @param eip new user instruction pointer.  Set only if != 0xffffffff.
    @param esp new user stack pointer.  Set only if != 0xffffffff.
    @param o_eip return current instruction pointer if pointer != 0
    @param o_esp return current stack pointer if pointer != 0
    @param o_pager return current pager if pointer != 0
    @param o_preempter return current internal preempter if pointer != 0
    @param o_eflags return current eflags register if pointer != 0
    @return false if !exists(); true otherwise
 */
PUBLIC
bool
Thread::initialize(Address ip, Address sp,
		   Thread* pager, Receiver* preempter,
		   Address *o_ip = 0, Address *o_sp = 0,
		   Thread* *o_pager = 0, Receiver* *o_preempter = 0,
		   Address *o_flags = 0,
		   bool no_cancel = 0,
		   bool alien = 0)
{
  assert (current() == thread_lock()->lock_owner());

  if (state() == Thread_invalid)
    return false;

  reload_ip_sp_from_utcb();

  Entry_frame *r = regs();

  if (o_pager) *o_pager = _pager;
  if (o_preempter) *o_preempter = preemption()->receiver();
  if (o_sp) *o_sp = r->sp();
  if (o_ip) *o_ip = r->ip();
  if (o_flags) *o_flags = r->flags();

  if (ip != ~0UL)
    {
      // We have to consider a special case where we have to leave the kernel
      // with iret instead of sysexit: If the target thread entered the kernel
      // through sysenter, it would leave using sysexit. This is not possible
      // for two reasons: Firstly, the sysexit instruction needs special user-
      // land code to load the right value into the edx register (see user-
      // level sysenter bindings). And secondly, the sysexit instruction
      // decrements the user-level eip value by two to ensure that the fixup
      // code is executed. One solution without kernel support would be to add
      // the instructions "movl %ebp, %edx" just _before_ the code the target
      // eip is set to.
      if (r->cs() & 0x80)
	{
	  // this cannot happen in Fiasco UX
	  extern Mword leave_from_sysenter_by_iret;
	  Mword **ret_from_disp_syscall = reinterpret_cast<Mword**>(regs())-1;
	  r->cs(r->cs() & ~0x80);
	  *ret_from_disp_syscall = &leave_from_sysenter_by_iret;
	}
      r->ip(ip);

      if (alien)
	state_change (~Thread_dis_alien, Thread_alien);
      else
	state_del (Thread_alien);

      if (! (state() & Thread_dead))
	{
#if 0
	  kdb_ke("reseting non-dead thread");
#endif
	  if (!no_cancel)
	    // cancel ongoing IPC or other activity
	    state_change (~(Thread_ipc_in_progress | Thread_delayed_deadline |
                          Thread_delayed_ipc), Thread_cancel | Thread_ready);
	}
      else
	state_change (~Thread_dead, Thread_ready);
    }

  if (pager != 0) _pager = pager;
  if (preempter != 0) preemption()->set_receiver (preempter);
  if (sp != ~0UL) r->sp(sp);

  return true;
}

IMPLEMENT inline
void
Thread::rcv_startup_msg()
{}

PRIVATE inline
void
Thread::enqueue_thread0_other_task()
{
  // other task -> enqueue in front of this task
  present_enqueue
    (lookup_first_thread
     (Thread::lookup (thread_lock()->lock_owner())
      ->space_index())->present_prev);
  // that's safe because thread 0 of a task is always present
}

PRIVATE inline
int
Thread::is_privileged_for_debug (Trap_state * /*ts*/)
{
#if 0
  return ((ts->eflags & EFLAGS_IOPL) == EFLAGS_IOPL_U);
#else
  return 1;
#endif
}

/** Check if the pagefault occured at a special place: At some places in the
    kernel we want to ensure that a specific address is mapped. The regular
    case is "mapped", the exception or slow case is "not mapped". The fastest
    way to check this is to touch into the memory. If there is no mapping for
    the address we get a pagefault. Now the pagefault exception handler can
    recognize that situation by scanning the code. The trick is that the
    assembler instruction "andl $0xffffffff, %ss:(%ecx)" _clears_ the carry
    flag normally (see Intel reference manual). The pager wants to inform the
    code that there was a pagefault and therefore _sets_ the carry flag. So
    the code has only to check if the carry flag is set. If yes, there was
    a pagefault at this instruction.
    @param ip pagefault address */
IMPLEMENT inline NOEXPORT
Mword
Thread::pagein_tcb_request (Address eip)
{
  return *(Unsigned32*)eip == 0xff218336;
}

IMPLEMENT inline
Mword
Thread::is_tcb_mapped() const
{
  // Touch the state to page in the TCB. If we get a pagefault here,
  // the handler doesn't handle it but returns immediatly after
  // setting eax to 0xffffffff
  Mword pagefault_if_0;
  asm volatile ("xorl %%eax,%%eax		\n\t"
		"andl $0xffffffff, %%ss:(%%ecx)	\n\t"
		"setnc %%al			\n\t"
		: "=a" (pagefault_if_0) : "c" (&_state));
  return pagefault_if_0;
}



PRIVATE static 
void
Thread::print_page_fault_error(Mword e)
{
  printf("%lx", e);
}

/**
 * The global trap handler switch.
 * This function handles CPU-exception reflection, emulation of CPU 
 * instructions (LIDT, WRMSR, RDMSR), int3 debug messages, 
 * kernel-debugger invocation, and thread crashes (if a trap cannot be 
 * handled).
 * @param state trap state
 * @return 0 if trap has been consumed by handler;
 *          -1 if trap could not be handled.
 */    
PUBLIC
int
Thread::handle_slow_trap (Trap_state *ts)
{ 
  Address ip;
  int from_user = ts->cs & 3;

  if (EXPECT_FALSE (gdb_trap_recover))
    goto generic_debug;		// we're in the GDB stub or in jdb 
                                // -- let generic handler handle it

  LOG_TRAP;

  if (!check_trap13_kernel (ts, from_user))
    return 0;

  if (EXPECT_FALSE (! from_user))
    {
      // small space faults can be raised in kernel mode, too (long IPC)
      if (ts->trapno == 13 && (ts->err & 0xfffff) == 0 && 
	  handle_smas_gp_fault ())
	goto success;

      // get also here if a pagefault was not handled by the user level pager
      if (ts->trapno == 14)
	goto pf_in_kernel;

      goto generic_debug;      // we were in kernel mode -- nothing to emulate
    }

  if (EXPECT_FALSE (ts->trapno == 2) )
    goto generic_debug;        // NMI always enters kernel debugger

  if (EXPECT_FALSE (ts->trapno == 0xffffffff))
    goto generic_debug;        // debugger interrupt

  check_f00f_bug (ts);

  // so we were in user mode -- look for something to emulate
  
  // We continue running with interrupts off -- no sti() here. But
  // interrupts may be enabled by the pagefault handler if we get a
  // pagefault in peek_user().

  // Set up exception handling.  If we suffer an un-handled user-space
  // page fault, kill the thread.
  jmp_buf pf_recovery; 
  unsigned error;
  if (EXPECT_FALSE ((error = setjmp(pf_recovery)) != 0) )
    {
      WARN ("%x.%x (tcb="L4_PTR_FMT") killed:\n"
	    "\033[1mUnhandled page fault, code=%08x\033[m\n",
	    d_taskno(), d_threadno(), (Address)this, error);
      goto fail_nomsg;
    }

  _recover_jmpbuf = &pf_recovery;
  ip = ts->ip();

  if (handle_io_page_fault (ts, ip, from_user))
    goto success;

  // check for "invalid opcode" exception
  if (EXPECT_FALSE (Config::Kip_syscalls && ts->trapno == 6))
    {
      // Check "lock; nop" opcode
      if (space()->peek ((Unsigned16*) ip, from_user) == 0x90f0)
        {
          cas ((Address*)(&ts->eip), ip, ip + 2);// step behind the opcode
	  ts->eax = space()->kip_address();
          goto success;
        }
    }

  // just print out some warning, we do the normal exception handling
  handle_sysenter_trap (ts, ip, from_user);

  // check for general protection exception
  if (ts->trapno == 13 && (ts->err & 0xffff) == 0)
    {
      // find out if we are a privileged task
      bool is_privileged = trap_is_privileged (ts);

      // check for "lidt (%eax)"
      if (EXPECT_FALSE
	  (ip < Kmem::mem_user_max - 4 &&
	   (space()->peek ((Mword*) ip, from_user) & 0xffffff) == 0x18010f))
	{
          // emulate "lidt (%eax)"

          // read descriptor
          if (ts->eax >= Kmem::mem_user_max - 6)
            goto fail;

          Idt_entry *idt = space()->peek((Idt_entry**)(ts->eax+2), from_user);
  	  Unsigned16 limit = Config::backward_compatibility
	    ? 255 : space()->peek ((Unsigned16*) ts->eax, from_user);

          if (reinterpret_cast<Address>(idt) >= Kmem::mem_user_max-limit-1)
            goto fail;

          // OK; store descriptor
          _idt = idt;
          _idt_limit = (limit + 1) / sizeof(Idt_entry);

          // consume instruction and continue
          cas ((Address*)(&ts->eip), ip, ip + 3); // ignore errors
          goto success;
        }

      // check for "lldt %ax"
      if (handle_lldt (ts))
	goto success;

      // check for "wrmsr (%eax)"
      if (EXPECT_FALSE
	  (is_privileged
	   && (ip < Kmem::mem_user_max - 2)
	   && (space()->peek ((Unsigned16*) ip, from_user)) == 0x300f
	   && (Cpu::features() & FEAT_MSR)))
        {
	  do_wrmsr_in_kernel (ts);

          // consume instruction and continue
          cas ((Address*)(&ts->eip), ip, ip + 2); // ignore errors
          goto success;
        }

      // check for "rdmsr (%eax)"
      if (EXPECT_FALSE
	  (is_privileged
	   && (ip < Kmem::mem_user_max - 2)
	   && (space()->peek ((Unsigned16*) ip, from_user)) == 0x320f
	   && (Cpu::features() & FEAT_MSR)))
        {
	  do_rdmsr_in_kernel (ts);

          // consume instruction and continue
          cas ((Address*)(&ts->eip), ip, ip + 2); // ignore errors
          goto success;
        }

      // check for "hlt" -> deliver L4 version
      if (is_privileged
	  && (ip < Kmem::mem_user_max - 1)
	  && (space()->peek ((Unsigned8*) ip, from_user) == 0xf4))
	{
          // FIXME: What version should Fiasco deliver?
	  ts->eax = 0x00010000;

	  // consume instruction and continue
          cas ((Address*)(&ts->eip), ip, ip + 1); // ignore errors
          goto success;
	}

      if (handle_smas_gp_fault ())
	goto success;
    }

pf_in_kernel:

  // send exception IPC if requested
  if (snd_exception(ts))
    goto success;

  // let's see if we have a trampoline to invoke
  if (ts->trapno < 0x20 && ts->trapno < _idt_limit)
    {
      Idt_entry e = space()->peek (_idt + ts->trapno, 1);

      if (Config::backward_compatibility // backward compat.: don't check
          || (   (e.word_count() & 0xe0) == 0x00
              && (e.access()     & 0x1f) == 0x0f)) // gate descriptor ok?
        {
          Address handler = e.offset();

          if ((handler || !Config::backward_compatibility) //bw compat.: != 0?
              && handler < Kmem::mem_user_max // in user space?
              && ts->esp <= Kmem::mem_user_max
              && ts->esp > 5 * sizeof (Mword)) // enough space on user stack?
            {
	      // OK, reflect the trap to user mode
	      if (!raise_exception (ts, handler))
                {
                  // someone interfered and changed our state
		  assert (state() & Thread_cancel);
                  state_del (Thread_cancel);
                }

              goto success;     // we've consumed the trap
            }
        }
    }

  // backward compatibility cruft: check for those insane "int3" debug
  // messaging command sequences
  if ((ts->trapno == 3) && is_privileged_for_debug (ts))
    {
      if (int3_handler && int3_handler(ts))
	goto success;

      goto generic_debug;
    }

  // privileged tasks also may invoke the kernel debugger with a debug
  // exception
  if ((ts->trapno == 1) && is_privileged_for_debug (ts))
    goto generic_debug;

fail:
  // can't handle trap -- kill the thread
  WARN ("%x.%x (tcb=%08x) killed:\n"
	"\033[1mUnhandled trap\033[m\n",
	d_taskno(), d_threadno(), (unsigned) this);

fail_nomsg:
  if (Config::warn_level >= Warning)
    ts->dump();

  if (Config::conservative)
    kdb_ke ("thread killed");

  halt();

success:
  _recover_jmpbuf = 0;
  return 0;

generic_debug:
  _recover_jmpbuf = 0;

  if (!nested_trap_handler)
    return handle_not_nested_trap (ts);

  return call_nested_trap_handler (ts);
}

/**
 * The low-level page fault handler called from entry.S.  We're invoked with
 * interrupts turned off.  Apart from turning on interrupts in almost
 * all cases (except for kernel page faults in TCB area), just forwards
 * the call to Thread::handle_page_fault().
 * @param pfa page-fault virtual address
 * @param error_code CPU error code
 * @return true if page fault could be resolved, false otherwise
 */
extern "C" FIASCO_FASTCALL
int
thread_page_fault (Address pfa, Mword error_code, Address ip, Mword flags,
		   Return_frame *regs)
{
  // If we're in the GDB stub -- let generic handler handle it
  if (EXPECT_FALSE (gdb_trap_recover))
    return false;

  // Pagefault in user mode or interrupts were enabled
  if (PF::is_usermode_error(error_code) || (flags & EFLAGS_IF))
    Proc::sti();

  // Pagefault in kernel mode and interrupts were disabled
  else 
    {
      // page fault in kernel memory region, not present, but mapping exists
      if (Kmem::is_kmem_page_fault (pfa, error_code))
	{
	  // We've interrupted a context in the kernel with disabled interrupts,
	  // the page fault address is in the kernel region, the error code is
	  // "not mapped" (as opposed to "access error"), and the region is
	  // actually valid (that is, mapped in Kmem's shared page directory,
	  // just not in the currently active page directory)
	  // Remain cli'd !!!

	  // Test for special case -- see function documentation
	  if (Thread::pagein_tcb_request(ip))
	    {
	      // skip faulting instruction
	      kdb_ke("stop");
	      regs->ip(regs->ip() + 4);
	      // tell program that a pagefault occured we cannot handle
	      regs->flags(regs->flags() | 1);	// set carry flag in EFLAGS
	      return 2; // handled, set carry flag
	    }

	  // enable interrupts only if there is no presend mapping 
	  // in the master directory
	  if (!PF::is_translation_error(error_code) ||
	      !Kmem::virt_to_phys (reinterpret_cast<void*>(pfa)) != (Mword) -1)
	    Proc::sti();
	} 
      else if (!Config::conservative &&
	       !Kmem::is_kmem_page_fault (pfa, error_code))
	{
      	  // No error -- just enable interrupts.
	  Proc::sti();
	} 
      else 
	{
       	  // Error: We interrupted a cli'd kernel context touching kernel space
	  if (!Thread::log_page_fault())
	    printf("*P[%lx,%lx,%lx] ", pfa, error_code & 0xffff, ip);

	  kdb_ke ("page fault in cli mode");
	}
    }

  return current_thread()->handle_page_fault (pfa, error_code, ip);
}


/** Raise a machine exception for the task.
    This function modifies a thread's user stack pointer and user instruction 
    pointer to emulate a machine exception.
    @param ts trap state that is being posted
    @param handler pointer to user-level handler function the thread has 
                   installed
    @return false if thread is not ready or has been 
            reinititialized using initialize().  Otherwise true.
 */
PUBLIC inline NEEDS ["trap_state.h"]
bool
Thread::raise_exception (Trap_state *ts, Address handler)
{
  Mword ip = ts->ip(), *sp = (Mword *) ts->sp();
  bool error_code = false, fault_addr = false;

  {
    Lock_guard <Thread_lock> guard (thread_lock());

    if (!(state() & Thread_ready) || (state() & Thread_cancel))
      return false;

    switch (ts->trapno)
      {
        case 0xe:
          fault_addr = true;
        case 0x8:
        case 0xa:
        case 0xb:
        case 0xc:
        case 0xd:
        case 0x11:
          error_code = true;
      }

    ts->esp -= sizeof (Mword) * (error_code ? 4 : 3);
    ts->eip  = handler;
  }

  space()->poke_user (sp - 1, (Mword)ts->flags());
  space()->poke_user (sp - 2, (Mword)exception_cs());
  space()->poke_user (sp - 3, (Mword)ip);

  if (error_code)
    space()->poke_user (sp - 4, (Mword)ts->err);
  if (fault_addr)
    space()->poke_user (sp - 5, (Mword)ts->cr2);

  /* reset single trap flag to mirror cpu correctly */
  if (ts->trapno == 1)
    ts->eflags &= ~EFLAGS_TF;

  return true;
}



/** The catch-all trap entry point.  Called by assembly code when a 
    CPU trap (that's not specially handled, such as system calls) occurs.
    Just forwards the call to Thread::handle_slow_trap().
    @param state trap state
    @return 0 if trap has been consumed by handler;
           -1 if trap could not be handled.
 */
extern "C" FIASCO_FASTCALL
int 
thread_handle_trap(Trap_state *ts)
{
  return current_thread()->handle_slow_trap(ts);
}

// We are entering with disabled interrupts!
extern "C" FIASCO_FASTCALL
void
thread_timer_interrupt (Address ip)
{
#if defined(CONFIG_IA32) && defined(CONFIG_PROFILE)
  cpu_lock.lock();
#endif
  (void)ip;

  Timer::acknowledge();
  Timer::update_system_clock();

  LOG_TIMER_IRQ (Config::scheduler_irq_vector);

  current_thread()->handle_timer_interrupt();

#if defined(CONFIG_IA32) && defined(CONFIG_PROFILE)
  cpu_lock.clear();
#endif
}

// 
// Public services
// 

IMPLEMENT inline
bool
Thread::handle_sigma0_page_fault (Address pfa)
{
  size_t size;

  // Check if mapping a superpage doesn't exceed the size of physical memory
  if (Cpu::have_superpages() &&
      (pfa & Config::SUPERPAGE_MASK) + Config::SUPERPAGE_SIZE <
       Kip::k()->main_memory_high())
    {
      pfa &= Config::SUPERPAGE_MASK;
      size = Config::SUPERPAGE_SIZE;
    }
  else
    {
      pfa &= Config::PAGE_MASK;
      size = Config::PAGE_SIZE;
    }

  return space()->v_insert (pfa, pfa, size,
                            Space::Page_writable |
                            Space::Page_user_accessible)
                         != Space::Insert_err_nomem;
}

IMPLEMENT inline NEEDS ["config.h", "space.h", "std_macros.h"]
Mword
Thread::update_ipc_window (Address pfa, Address remote_pfa, Mword error_code)
{
  Space *remote = receiver()->space();
  
  // If the remote address space has a mapping for the page fault address and
  // it is writable or we didn't want to write to it, then we can simply copy
  // the mapping into our address space via space()->remote_update().
  // Otherwise return 0 to trigger a pagein_request upstream.
  
  if (EXPECT_TRUE (remote->mapped (remote_pfa, (error_code & PF_ERR_WRITE))))
    {
      //careful: for SMAS current_space() != space()
      current_space()->remote_update (pfa, remote, remote_pfa, 1);

      // It's OK if the PF occurs again: This can happen if we're
      // preempted after the call to remote_update() above.  (This code
      // corresponds to code in Thread::handle_page_fault() that
      // checks for double page faults.)

      if (Config::monitor_page_faults)
        _last_pf_address = (Address) -1;

      return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
IMPLEMENTATION[{ia32,ux}-!io]: 

PRIVATE inline
bool
Thread::get_ioport(Address /*eip*/, Trap_state * /*ts*/,
                   unsigned * /*port*/, unsigned * /*size*/)
{
  return false;
}


//-----------------------------------------------------------------------------
IMPLEMENTATION [syscall_iter]:

PRIVATE inline
void
Thread::enqueue_thread_other_task()
{
  if (id().lthread())
    {
      // other task and non-first thread
      // --> enqueue in the destination address space after the first
      //     thread
      present_enqueue (lookup_first_thread(space()->id()));
    }
  else
    enqueue_thread0_other_task();
}


//-----------------------------------------------------------------------------
IMPLEMENTATION [!syscall_iter]:

PRIVATE inline
void
Thread::enqueue_thread_other_task()
{
  enqueue_thread0_other_task();
}


//-----------------------------------------------------------------------------
IMPLEMENTATION [{ia32,ux}-segments]:

#include "warn.h"

/**
 * Extract parameters for the TLS handling.
 *
 * \param  ts		trap state
 * \retval desc_addr	address of the descriptors
 * \retval size		size of descritptors (multiple of 8)
 * \retval entry_number	number of first entry
 * \retval task		task number
 * \return		0 on success */
PRIVATE inline
int
Thread::tls_setup_emu(Trap_state *ts, Address *desc_addr, int *size,
		      unsigned *entry_number, Thread **t)
{
  if (ts->edx != 0)
    return 1;

  *desc_addr    = ts->eax;
  *size         = ts->ebx;
  *entry_number = ts->ecx;

  // In any case, jump over the instruction
  ts->eip += 3;

  *t = lookup(L4_uid(((Unsigned64)ts->edi << 32) | ts->esi));
  if (!*t)
    {
      WARN("set_tls: non-existing thread %08lx.%08lx", ts->edi, ts->esi);
      return 1;
    }
  if ((*entry_number<<3) + *size > 3*Cpu::Ldt_entry_size)
    {
      WARN("set_tls: entry/size out of range.");
      return 1;
    }

  return 0;
}

/**
 * Extract parameters for the LDT handling.
 *
 * \param ts		trap state
 * \retval s		space
 * \retval desc_addr	address of the descriptors
 * \retval size		size of descritptors (multiple by 8)
 * \retval entry_number	number of first entry
 * \retval task		task number
 * \return 		0 on success */
PRIVATE inline NEEDS ["warn.h"]
int
Thread::lldt_setup_emu(Trap_state *ts, Space **s, Address *desc_addr,
		       int *size, unsigned *entry_number, Task_num *task)
{
  *desc_addr    = ts->eax;
  *size         = ts->ebx;
  *entry_number = ts->ecx;
  *task         = ts->edx;

  // In any case, jump over the instruction
  ts->eip += 3;

  // Check if address space exists
  *s = Space_index(*task).lookup();
  if (!*s)
    {
      WARN("set_ldt: non-existing task %x.", *task);
      return 1;
    }

  // Check that only the pager and one itself can set the LDT
  Thread *t = Thread::lookup(L4_uid(*task, 0));
  if (id().task() != *task && t->_pager->id().task() != id().task())
    {
      WARN("set_ldt: %x.%x is not pager of %x.0.",
	   id().task(), id().lthread(), t->id().task());
      return 1;
    }

  return 0;
} 


//---------------------------------------------------------------------------
IMPLEMENTATION[{ia32,ux}-utcb]: 

#include "utcb.h"

/** Setup the UTCB pointer of this thread.
 */
PRIVATE inline NOEXPORT
void
Thread::setup_utcb_kernel_addr()
{
  Address uaddr = Mem_layout::V2_utcb_addr + id().lthread()*sizeof(Utcb);
  Utcb *ptr     = reinterpret_cast<Utcb *>
                   (Kmem::phys_to_virt(space()->virt_to_phys(uaddr)));

  local_id((Local_id)uaddr);
  utcb(ptr);

  setup_exception_ipc();
  setup_lipc_utcb();
}

IMPLEMENTATION [{ia32,ux}-!lipc]:

PRIVATE inline
void
Thread::setup_lipc_utcb()
{}

IMPLEMENTATION [{ia32,ux}-!exc_ipc]:

PRIVATE inline
void
Thread::setup_exception_ipc() 
{}


//---------------------------------------------------------------------------
IMPLEMENTATION[{ia32,ux}-!utcb]: 

/** Dummy function to hold code in thread-ia32-ux-v2x0 generic.
 */
PRIVATE inline NOEXPORT
void
Thread::setup_utcb_kernel_addr()
{}


//---------------------------------------------------------------------------
IMPLEMENTATION [{ia32,ux}-exc_ipc]:

PUBLIC inline
int
Thread::snd_exception(Trap_state *ts)
{
  if (//ts->trapno != 1 && ts->trapno != 3 &&
      _pager != kernel_thread &&
      _pager->utcb()->status)
    {

      if (ts->trapno == 3)
	{
	  if (!(state() & Thread_alien))
	    return 0;
	  if (state() & Thread_dis_alien)
	    {
	      state_del(Thread_dis_alien);
	      return 0;
	    }
	}

      state_del(Thread_cancel);
      Proc::sti();		// enable interrupts, we're sending IPC
      exception(ts);
      return 1;      // We did it
    }
  return 0;
}

PRIVATE 
Ipc_err
Thread::exception(Trap_state *ts)
{
  Sys_ipc_frame r;
  L4_timeout timeout( L4_timeout::Never );
  Thread *handler = _pager; // _exception_handler;
  void *old_utcb_handler = _utcb_handler;
  _utcb_handler = ts;

  // fill registers for IPC
  r.set_msg_word(0, L4_exception_ipc::Exception_ipc_cookie_1);
  r.set_msg_word(1, L4_exception_ipc::Exception_ipc_cookie_2);
  r.set_msg_word(2, 0); // nop in V2
  r.snd_desc(0);
  r.rcv_desc(0);

  prepare_receive(handler, &r);

  Ipc_err ret (0);

  Ipc_err err = do_send(handler, timeout, &r);

  if (EXPECT_FALSE(err.has_error()))
    {
      if (Config::conservative)
	{
	  printf (" page fault send error = 0x%lx\n", err.raw());
	  kdb_ke ("snd to pager failed");
	}
      
      // skipped receive operation
      state_del(Thread_ipc_receiving_mask);

      if (err.error() == Ipc_err::Enot_existent)
	ret = (state() & Thread_cancel)
	  ? Ipc_err (0) // retry user insn after thread_ex_regs
	  : err;
      // else ret = 0 -- that's the default, and it means: retry
    }
  else
    {
      err = do_receive(handler, timeout, &r);

      if (EXPECT_FALSE(err.has_error()))
	{
	  if (Config::conservative)
	    {
	      printf("page fault rcv error = 0x%lx\n", err.raw());
	      kdb_ke("rcv from pager failed");
	    }
	}
      // else ret = 0 -- that's the default, and it means: retry

      if (r.msg_word(0) == 1)
	state_add(Thread_dis_alien);
    }

  // restore original utcb_handler
  _utcb_handler = old_utcb_handler;

  return ret;
}

PRIVATE inline
void
Thread::setup_exception_ipc() 
{
  _utcb_handler = 0;
}


//-----------------------------------------------------------------------------
IMPLEMENTATION [{ia32,ux}-!exc_ipc]:

PUBLIC inline
int
Thread::snd_exception(Trap_state *) const
{
  return 0;
}

