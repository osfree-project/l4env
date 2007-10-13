/*
 * Fiasco Thread Code
 * Shared between UX and native IA32.
 */
INTERFACE [ia32,amd64,ux]:

#include "trap_state.h"

class Idt_entry;

EXTENSION class Thread 
{
private:
  /**
   * Return code segment used for exception reflection to user mode
   */
  static Mword exception_cs();

protected:
  Idt_entry *_idt;
  Unsigned16 _idt_limit;

  static Trap_state::Handler nested_trap_handler FIASCO_FASTCALL;
};

//----------------------------------------------------------------------------
IMPLEMENTATION [ia32,amd64,ux]:

#include "config.h"
#include "cpu.h"
#include "cpu_lock.h"
#include "gdt.h"
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
        _deadline_timeout (&_preemption)
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
  _exc_ip         = ~0UL;

  *reinterpret_cast<void(**)()> (--_kernel_sp) = user_invoke;

  arch_init();
  setup_utcb_kernel_addr();

  caps_init (current_thread());
  _pager = _ext_preempter = nil_thread;

  preemption()->set_receiver (nil_thread);

  present_enqueue();

  state_add(Thread_dead);
  
  // ok, we're ready to go!
}

IMPLEMENT inline NEEDS[Thread::exception_triggered]
Mword
Thread::user_ip() const
{ return exception_triggered()?_exc_ip:regs()->ip(); }

IMPLEMENT inline
Mword
Thread::user_flags() const
{ return regs()->flags(); }



PRIVATE inline
int
Thread::is_privileged_for_debug (Trap_state * /*ts*/)
{
#if 0
  return ((ts->flags() & EFLAGS_IOPL) == EFLAGS_IOPL_U);
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
IMPLEMENT inline
bool
Thread::pagein_tcb_request (Return_frame *regs)
{
  unsigned long new_ip = regs->ip();
  if (*(Unsigned8*)new_ip == 0x48) // REX.W
    new_ip += 1;

  register Unsigned16 op = *(Unsigned16*)new_ip;
  //LOG_MSG_3VAL(current(),"TCB", op, regs->ip(), 0);
  if ((op & 0xc0ff) == 0x8b) // Context::is_tcb_mapped() and Context::state()
    {
      regs->ip(new_ip + 2);
      // stack layout:
      //         user eip
      //         PF error code
      // reg =>  eax/rax
      //         ecx/rcx
      //         edx/rdx
      //         ...
      Mword *reg = ((Mword*)regs) - 2;
#if 0
      LOG_MSG_3VAL(current(),"TCB", op, regs->ip(), (Mword)reg);
      LOG_MSG_3VAL(current(),"TCBX", reg[-3], reg[-4], reg[-5]);
      LOG_MSG_3VAL(current(),"TCB0", reg[0], reg[-1], reg[-2]);
      LOG_MSG_3VAL(current(),"TCB1", reg[1], reg[2], reg[3]);
#endif
      assert((op >> 11) <= 2);
      reg[-(op>>11)] = 0; // op==0 => eax, op==1 => ecx, op==2 => edx

      // tell program that a pagefault occured we cannot handle
      regs->flags(regs->flags() | 0x41); // set carry and zero flag in EFLAGS
      return true;
    }
  else if (*(Unsigned32*)regs->ip() == 0xff01f636) // used in shortcut.S
    {
      regs->ip(regs->ip() + 4);
      regs->flags(regs->flags() | 1);  // set carry flag in EFLAGS
      return true;
    }

  return false;
}

IMPLEMENTATION[ia32 || ux]:

IMPLEMENT inline
Mword 
Thread::user_sp() const
{ return regs()->sp(); }

IMPLEMENT inline
void
Thread::user_sp(Mword sp)
{ regs()->sp(sp); }

PRIVATE inline
int
Thread::do_trigger_exception(Entry_frame *r)
{
  if (!exception_triggered())
    {
      extern Mword leave_by_trigger_exception;
      register Address xip = r->ip();
      user_ip(reinterpret_cast<Address>(&leave_by_trigger_exception));
      _exc_ip = xip;
      r->cs (Gdt::gdt_code_kernel | Gdt::Selector_kernel);
      return 1;
    }
  // else ignore change of IP because triggered exception already pending
  return 0;
}


PUBLIC inline NOEXPORT
void
Thread::restore_exc_state()
{
  Entry_frame *r = regs();

#ifdef CONFIG_PF_UX
  r->cs (exception_cs() & ~1);
#else
  r->cs (exception_cs());
#endif
  r->ip (_exc_ip);
  _exc_ip = ~0UL;
}

IMPLEMENTATION[(ia32 || ux) && segments]:

PRIVATE static inline
void
Thread::copy_utcb_to_ts_reset_segments(Thread *rcv)
{ rcv->_gs = rcv->_fs = 0; }

IMPLEMENTATION[(ia32 || ux) && !segments]:

PRIVATE static inline
void
Thread::copy_utcb_to_ts_reset_segments(Thread *)
{}

IMPLEMENTATION[ia32 || ux]:


PRIVATE static inline
void
Thread::copy_utcb_to_ts(L4_msg_tag const &tag, Thread *snd, Thread *rcv)
{
  Trap_state *ts = (Trap_state*)rcv->_utcb_handler;
  Mword       s  = tag.words();
  Unsigned32  cs = ts->cs();

  if (EXPECT_FALSE(rcv->exception_triggered()))
    {
      // triggered exception pending
      Cpu::memcpy_mwords (&ts->_gs, snd->utcb()->values, s > 12 ? 12 : s);
      if (EXPECT_TRUE(s > 12))
	rcv->_exc_ip = snd->utcb()->values[12];
      if (EXPECT_TRUE(s > 14))
	ts->flags(snd->utcb()->values[14]);
      if (EXPECT_TRUE(s > 15))
	ts->sp(snd->utcb()->values[15]);
    }
  else
    Cpu::memcpy_mwords (&ts->_gs, snd->utcb()->values, s > 16 ? 16 : s);

  copy_utcb_to_ts_reset_segments(rcv);

  if (tag.transfer_fpu())
    snd->transfer_fpu(rcv);

  // sanitize eflags
  if (!rcv->trap_is_privileged(0))
    ts->flags((ts->flags() & ~(EFLAGS_IOPL | EFLAGS_NT)) | EFLAGS_IF);

  // don't allow to overwrite the code selector!
  ts->cs(cs);

  rcv->state_del(Thread_in_exception);
}

PRIVATE static inline
void
Thread::copy_ts_to_utcb(L4_msg_tag const &, Thread *snd, Thread *rcv)
{
  Trap_state *ts = (Trap_state*)snd->_utcb_handler;
  Mword        r = Utcb::Max_words;

  {
    Lock_guard <Cpu_lock> guard (&cpu_lock);
    if (EXPECT_FALSE(snd->exception_triggered()))
      {
	Cpu::memcpy_mwords (rcv->utcb()->values, &ts->_gs, r > 12 ? 12 : r);
	if (EXPECT_TRUE(r > 12))
	  rcv->utcb()->values[12] = snd->_exc_ip;
	if (EXPECT_TRUE(r > 14))
	  rcv->utcb()->values[14] = ts->flags();
	if (EXPECT_TRUE(r > 15))
	  rcv->utcb()->values[15] = ts->sp();
      }
    else
      Cpu::memcpy_mwords (rcv->utcb()->values, &ts->_gs, r > 16 ? 16 : r);

    if (rcv->utcb()->inherit_fpu())
	snd->transfer_fpu(rcv);

  }
}


//----------------------------------------------------------------------------
IMPLEMENTATION [ux || amd64]:

IMPLEMENT inline NEEDS[Thread::exception_triggered]
void
Thread::user_ip(Mword ip)
{ 
  if (exception_triggered())
    _exc_ip = ip;
  else
    {
      Entry_frame *r = regs();
      r->ip(ip);
    }
}


//----------------------------------------------------------------------------
IMPLEMENTATION [ia32 && !ux]:

IMPLEMENT inline NEEDS[Thread::exception_triggered]
void
Thread::user_ip(Mword ip)
{ 
  if (exception_triggered())
    _exc_ip = ip;
  else
    {
      Entry_frame *r = regs();
      r->ip(ip);
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
	  /* symbols from the assember entry code */
	  extern Mword leave_from_sysenter_by_iret;
	  extern Mword leave_alien_from_sysenter_by_iret;
	  extern Mword ret_from_fast_alien_ipc;
	  Mword **ret_from_disp_syscall = reinterpret_cast<Mword**>(r)-1;
	  r->cs(r->cs() & ~0x80);
	  if (*ret_from_disp_syscall == &ret_from_fast_alien_ipc)
	    *ret_from_disp_syscall = &leave_alien_from_sysenter_by_iret;
	  else
	    *ret_from_disp_syscall = &leave_from_sysenter_by_iret;
	}
    }
}


//----------------------------------------------------------------------------
IMPLEMENTATION [(ia32,amd64,ux) && !io]:

PRIVATE inline
int
Thread::handle_io_page_fault (Trap_state *, bool)
{ return 0; }


//----------------------------------------------------------------------------
IMPLEMENTATION [ia32,amd64,ux]:

#include "idt.h"
#include "task.h"

extern "C" FIASCO_FASTCALL
void
thread_restore_exc_state()
{
  current_thread()->restore_exc_state();
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
  int from_user = ts->cs() & 3;

  // XXX We might be forced to raise an excepton. In this case, our return
  // CS:IP points to leave_by_trigger_exception() which will trigger the
  // exception just before returning to userland. But if we were inside an
  // IPC while we was ex-regs'd, we will generate the 'exception after the
  // syscall' _before_ we leave the kernel.
  if (ts->_trapno == 13 && (ts->_err & 6) == 6)
    goto check_exception;

  if (EXPECT_FALSE (gdb_trap_recover))
    goto generic_debug;		// we're in the GDB stub or in jdb 
                                // -- let generic handler handle it

  LOG_TRAP;

  if (!check_trap13_kernel (ts, from_user))
    return 0;

  if (EXPECT_FALSE (!from_user))
    {
      // small space faults can be raised in kernel mode, too (long IPC)
      if (ts->_trapno == 13 && (ts->_err & 0xfffff) == 0 && 
	  handle_smas_gp_fault ())
	goto success;

      // get also here if a pagefault was not handled by the user level pager
      if (ts->_trapno == 14)
	goto check_exception;

      goto generic_debug;      // we were in kernel mode -- nothing to emulate
    }

  if (EXPECT_FALSE (ts->_trapno == 2))
    goto generic_debug;        // NMI always enters kernel debugger

  if (EXPECT_FALSE (ts->_trapno == 0xffffffff))
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

  switch (handle_io_page_fault (ts, from_user))
    {
    case 1: goto success;
    case 2: goto fail;
    }
  
  ip = ts->ip();

  // check for "invalid opcode" exception
  if (EXPECT_FALSE (Config::Kip_syscalls && ts->_trapno == 6))
    {
      // Check "lock; nop" opcode
      if (mem_space()->peek ((Unsigned16*) ip, from_user) == 0x90f0)
        {
	  ts->consume_instruction(2);
	  ts->value(task()->map_kip());
          goto success;
        }
    }

  // just print out some warning, we do the normal exception handling
  handle_sysenter_trap (ts, ip, from_user);

  // check for general protection exception
  if (ts->_trapno == 13 && (ts->_err & 0xffff) == 0)
    {
      // find out if we are a privileged task
      bool is_privileged = trap_is_privileged (ts);

      // check for "lidt (%eax)"
      if (EXPECT_FALSE
	  (ip < Kmem::mem_user_max - 4 &&
	   (mem_space()->peek((Mword*) ip, from_user) & 0xffffff) == 0x18010f))
	{
          // emulate "lidt (%eax)"

          // read descriptor
          if (ts->value() >= Kmem::mem_user_max - 6)
            goto fail;

          Idt_entry *idt = mem_space()->peek((Idt_entry**)(ts->value() + 2), 
	      					       from_user);
  	  Unsigned16 limit = Config::backward_compatibility
	    ? 255 : mem_space()->peek ((Unsigned16*) ts->value(), from_user);

          if ((Address)idt >= (Address)Kmem::mem_user_max-limit-1)
            goto fail;

          // OK; store descriptor
          _idt = idt;
          _idt_limit = (limit + 1) / sizeof(Idt_entry);

          // consume instruction and continue
	  ts->consume_instruction(3);
          // ignore errors
          goto success;
        }

      // check for "lldt %ax"
      if (handle_lldt (ts))
	goto success;

      // check for "wrmsr (%eax)"
      if (EXPECT_FALSE
	  (is_privileged
	   && (ip < Kmem::mem_user_max - 2)
	   && (mem_space()->peek ((Unsigned16*) ip, from_user)) == 0x300f
	   && (Cpu::can_wrmsr())))
        {
	  do_wrmsr_in_kernel (ts);

          // consume instruction and continue
	  ts->consume_instruction(2);
          // ignore errors
          goto success;
        }

      // check for "rdmsr (%eax)"
      if (EXPECT_FALSE
	  (is_privileged
	   && (ip < Kmem::mem_user_max - 2)
	   && (mem_space()->peek ((Unsigned16*) ip, from_user)) == 0x320f
	   && (Cpu::can_wrmsr())))
        {
	  do_rdmsr_in_kernel (ts);

          // consume instruction and continue
	  ts->consume_instruction(2);
          // ignore errors
          goto success;
        }

      if (handle_smas_gp_fault ())
	goto success;
    }

check_exception:

  // send exception IPC if requested
  if (send_exception(ts))
    goto success;

  // let's see if we have a trampoline to invoke
  if (ts->_trapno < 0x20 && ts->_trapno < _idt_limit)
    {
      Idt_entry e = mem_space()->peek (_idt + ts->_trapno, 1);

      if (Config::backward_compatibility // backward compat.: don't check
          || (   (e.word_count() & 0xe0) == 0x00
              && (e.access()     & 0x1f) == 0x0f)) // gate descriptor ok?
        {
          Address handler = e.offset();

          if ((handler || !Config::backward_compatibility) //bw compat.: != 0?
              && handler  <  Kmem::mem_user_max // in user space?
              && ts->sp() <= Kmem::mem_user_max
              && ts->sp() > 5 * sizeof (Mword)) // enough space on user stack?
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
  if (ts->_trapno == 3 && is_privileged_for_debug (ts))
    {
      if (int3_handler && int3_handler(ts))
	goto success;

      goto generic_debug;
    }

  // privileged tasks also may invoke the kernel debugger with a debug
  // exception
  if (ts->_trapno == 1 && is_privileged_for_debug (ts))
    goto generic_debug;

fail:
  // can't handle trap -- kill the thread
  WARN ("%x.%x (tcb="L4_PTR_FMT") killed:\n"
	"\033[1mUnhandled trap \033[m\n",
	d_taskno(), d_threadno(), (Address) this);

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
      // page fault in kernel memory region
      if (Kmem::is_kmem_page_fault (pfa, error_code))
	{
	  // We've interrupted a context in the kernel with disabled interrupts,
	  // the page fault address is in the kernel region, the error code is
	  // "not mapped" (as opposed to "access error"), and the region is
	  // actually valid (that is, mapped in Kmem's shared page directory,
	  // just not in the currently active page directory)
	  // Remain cli'd !!!
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

  return current_thread()->handle_page_fault (pfa, error_code, ip, regs);
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
    Lock_guard <Thread_lock> guard;
    
    if (!guard.lock(thread_lock()))
      return false;

    if (!(state() & Thread_ready) || (state() & Thread_cancel))
      return false;

    switch (ts->_trapno)
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

    ts->sp(ts->sp() - sizeof(Mword) * (error_code ? 4 : 3));
    ts->ip(handler);
  }

  mem_space()->poke_user (sp - 1, (Mword)ts->flags());
  mem_space()->poke_user (sp - 2, (Mword)exception_cs());
  mem_space()->poke_user (sp - 3, (Mword)ip);

  if (error_code)
    mem_space()->poke_user (sp - 4, (Mword)ts->_err);
  if (fault_addr)
    mem_space()->poke_user (sp - 5, (Mword)ts->_cr2);

  /* reset single trap flag to mirror cpu correctly */
  if (ts->_trapno == 1)
    ts->flags(ts->flags() & ~EFLAGS_TF);

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

  Timer::acknowledge();
  Timer::update_system_clock();

  LOG_TIMER_IRQ (Config::scheduler_irq_vector);
  (void)ip;

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

  return mem_space()->v_insert (pfa, pfa, size,
				Mem_space::Page_writable |
				Mem_space::Page_user_accessible)
    != Mem_space::Insert_err_nomem;
}

IMPLEMENT inline NEEDS ["config.h", "space.h", "std_macros.h"]
Mword
Thread::update_ipc_window (Address pfa, Address remote_pfa, Mword error_code)
{
  Mem_space *remote = receiver()->mem_space();
  
  // If the remote address space has a mapping for the page fault address and
  // it is writable or we didn't want to write to it, then we can simply copy
  // the mapping into our address space via space()->remote_update().
  // Otherwise return 0 to trigger a pagein_request upstream.
  
  if (EXPECT_TRUE (remote->mapped (remote_pfa, (error_code & PF_ERR_WRITE))))
    {
      //careful: for SMAS current_mem_space() != space()
      current_mem_space()->remote_update (pfa, remote, remote_pfa, 1);

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


PRIVATE inline
Utcb *
Thread::access_utcb() const
{ return utcb(); }


PUBLIC inline
int
Thread::send_exception(Trap_state *ts)
{
  if (((state() & Thread_alien) || (ts->_trapno != 1 && ts->_trapno != 3))
      && _pager != kernel_thread)
    {
      if (ts->_trapno == 3)
	{
	  if (!(state() & Thread_alien))
	    return 0;

	  if (state() & Thread_dis_alien)
	    {
	      state_del(Thread_dis_alien);
	      return 0;
	    }

          // set IP back on the int3 instruction
	  ts->ip(ts->ip() - 1);
	}

      state_change(~Thread_cancel, Thread_in_exception);

      Proc::sti();		// enable interrupts, we're sending IPC
      Ipc_err err = exception(ts);

      if (err.has_error() && err.error() == Ipc_err::Enot_existent)
	return 0;

      return 1;      // We did it
    }
  return 0;
}


// used by the assembler shortcut
// uargh, crap preprocess it dont like the asm directive and
// the fastcall macro at the function definition
PUBLIC static inline
void 
Thread::switch_exception_context(Thread *sender, Thread* receiver)
//  asm("switch_exception_context_label") 
{
#ifdef CONFIG_HANDLE_SEGMENTS
  sender->store_segments();
  sender->switch_gdt_user_entries(receiver);
  receiver->load_segments();
  if (sender->space() != receiver->space())
    receiver->space()->mem_space()->switch_ldt();
#else
  (void)sender;
#endif
  Mem_layout::user_utcb_ptr(receiver->local_id());
}

extern "C"  FIASCO_FASTCALL __attribute__((section(".text.asmshortcut")))
void
handle_utcb_ipc(Thread *sender, Thread* receiver, Mword tag)
{
  //LOG_MSG_3VAL(sender, "UTCB", tag, 0, 0);
  sender->copy_utcb_to(L4_msg_tag(tag), receiver);
}

extern "C"  FIASCO_FASTCALL __attribute__((section(".text.asmshortcut")))
void
switch_exception_context_wrapper(Thread *sender, Thread* receiver, Mword tag)
{
  handle_utcb_ipc(sender, receiver, tag);
  Thread::switch_exception_context(sender, receiver);
}


//---------------------------------------------------------------------------
IMPLEMENTATION[{ia32,amd64,ux}-!io]: 

PRIVATE inline
bool
Thread::get_ioport(Address /*eip*/, Trap_state * /*ts*/,
                   unsigned * /*port*/, unsigned * /*size*/)
{
  return false;
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
 * \retval entry_number	number of first entry, valid values: 0-2
 * \retval task		task number
 * \return		0 on success */
PRIVATE inline
int
Thread::tls_setup_emu(Trap_state *ts, Address *desc_addr, int *size,
		      unsigned *entry_number, Thread **t)
{
  if (ts->_edx != 0)
    return 1;

  *desc_addr    = ts->_eax;
  *size         = ts->_ebx;
  *entry_number = ts->_ecx;

  // In any case, jump over the instruction
  ts->ip(ts->ip() + 3);

  *t = id_to_tcb(L4_uid(ts->_esi));
  if (!*t || (*t)->thread_lock()->lock() == Thread_lock::Invalid)
    {
      WARN("set_tls: non-existing thread %08lx", ts->_esi);
      return 1;
    }
  if ((*entry_number<<3) + *size > 3*Cpu::Ldt_entry_size)
    {
      WARN("set_tls: entry/size out of range.");
      (*t)->thread_lock()->clear();
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
Thread::lldt_setup_emu(Trap_state *ts, Mem_space **s, Address *desc_addr,
		       int *size, unsigned *entry_number, Task_num *task)
{
  *desc_addr    = ts->_eax;
  *size         = ts->_ebx;
  *entry_number = ts->_ecx;
  *task         = ts->_edx;

  // In any case, jump over the instruction
  ts->ip(ts->ip() + 3);

  // Check if address space exists
  Space *space = Space_index(*task).lookup();
  if (!space)
    {
      *s = 0;
      WARN("set_ldt: non-existing task %x.", *task);
      return 1;
    }
  *s = space->mem_space();

  // Check that only the pager and one itself can set the LDT
  Thread *t = Thread::id_to_tcb(L4_uid(*task, 0));
  if (id().task() != *task && t->_pager->id().task() != id().task())
    {
      WARN("set_ldt: %x.%x is not pager of %x.0.",
	   id().task(), id().lthread(), t->id().task());
      return 1;
    }

  return 0;
} 


//---------------------------------------------------------------------------
IMPLEMENTATION[ia32 || amd64]:

#include "fpu.h"
#include "fpu_alloc.h"

PUBLIC inline NEEDS["fpu.h", "fpu_alloc.h"]
void
Thread::transfer_fpu(Thread *to)
{
  if (to->fpu_state()->state_buffer())
    Fpu_alloc::free_state(to->fpu_state());

  to->fpu_state()->state_buffer(fpu_state()->state_buffer());
  fpu_state()->state_buffer(0);

  Fpu::disable(); // it will be reanabled in switch_fpu

  if (Fpu::owner() == to)
    {
      Fpu::set_owner(0);
      to->state_change_dirty (~Thread_fpu_owner, 0);
    }
  else if (Fpu::owner() == this)
    {
      state_change_dirty (~Thread_fpu_owner, 0);
      to->state_change_dirty (~0U, Thread_fpu_owner);
      Fpu::set_owner(to);
    }
}

//---------------------------------------------------------------------------
IMPLEMENTATION[ux]:

PUBLIC inline
void
Thread::transfer_fpu(Thread *)
{}

