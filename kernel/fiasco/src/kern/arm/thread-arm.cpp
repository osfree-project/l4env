INTERFACE [arm]:

class Trap_state;

EXTENSION class Thread
{
private:
  bool _in_exception;
};

IMPLEMENTATION [arm]:

#include <cassert>
#include <cstdio>
#include <feature.h>

#include "globals.h"
#include "k_sa1100.h"
#include "kmem_space.h"
#include "thread_state.h"
#include "types.h"
#include "vmem_alloc.h"
#include "pic.h"

KIP_KERNEL_FEATURE("exception_ipc");

enum {
  FSR_STATUS_MASK = 0x0d,
  FSR_TRANSL      = 0x05,
  FSR_DOMAIN      = 0x09,
  FSR_PERMISSION  = 0x0d,
};

PRIVATE static
void
Thread::print_page_fault_error(Mword e)
{
  char const *const excpts[] =
    { "reset","undef. insn", "swi", "pref. abort", "data abort",
      "XXX", "XXX", "XXX" };

  unsigned ex = (e >> 20) & 0x07;

  printf("(%lx) %s, %s(%c)",e & 0xff, excpts[ex],
         (e & 0x00010000)?"user":"kernel",
         (e & 0x00020000)?'r':'w');
}


//
// Public services
//

IMPLEMENT
void
Thread::user_invoke()
{
  //  printf("Thread: %p [state=%08x, space=%p]\n", current(), current()->state(), current_mem_space() );

  assert (current()->state() & Thread_ready);
#if 0
  printf("user_invoke of %p @ %08x sp=%08x\n",
	 current(),current()->regs()->ip(),
	 current()->regs()->sp() );
  //current()->regs()->sp(0x30000);
#endif

  register unsigned long r0 asm ("r0")
    = Kmem_space::kdir()->walk(Kip::k(),0,false,0).phys(Kip::k());

#if 1
  asm volatile
    ("  mov sp, %[stack_p]    \n"    // set stack pointer to regs structure
     "  mov r1, sp            \n"
     // TODO clean out user regs
     "  ldr lr, [r1], #4      \n"
     "  msr spsr, lr          \n"
     "  ldmia r1, {sp}^       \n"
     "  add sp,sp, #20        \n"
     "  ldr lr, [sp, #-4]     \n"
     "  movs pc, lr           \n"
     :
     :
     [stack_p] "r" (nonull_static_cast<Return_frame*>(current()->regs())),
               "r" (r0)
     );
#endif
  puts("should never be reached");
  while(1) {
    current()->state_del(Thread_ready);
    current()->schedule();
  };

  // never returns here
}

IMPLEMENT inline NEEDS["space.h", <cstdio>, "types.h" ,"config.h"]
bool Thread::handle_sigma0_page_fault( Address pfa )
{
  return (mem_space()->v_insert((pfa & Config::SUPERPAGE_MASK),
	(pfa & Config::SUPERPAGE_MASK),
	Config::SUPERPAGE_SIZE,
	Mem_space::Page_writable | Mem_space::Page_user_accessible 
	| Mem_space::Page_cacheable)
      != Mem_space::Insert_err_nomem);
}


extern "C" {

  /**
   * The low-level page fault handler called from entry.S.  We're invoked with
   * interrupts turned off.  Apart from turning on interrupts in almost
   * all cases (except for kernel page faults in TCB area), just forwards
   * the call to Thread::handle_page_fault().
   * @param pfa page-fault virtual address
   * @param error_code CPU error code
   * @return true if page fault could be resolved, false otherwise
   */
  Mword pagefault_entry(const Mword pfa, const Mword error_code,
                        const Mword pc, Return_frame *ret_frame)
  {
#if 0 // Doubvle PF detect
    static unsigned long last_pfa = ~0UL;
    LOG_MSG_3VAL(current(),"PF", pfa, last_pfa, 0);
    if (last_pfa == pfa)
      kdb_ke("DBF");
    last_pfa = pfa;
#endif
    // Pagefault in user mode
    if (PF::is_usermode_error(error_code))
      {
	current_thread()->state_del(Thread_cancel);
        Proc::sti();
      }
    // or interrupts were enabled
    else if (!(ret_frame->psr & Proc::Status_IRQ_disabled))
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
	      printf("*P[%lx,%lx,%lx] ", pfa, error_code, pc);

	    kdb_ke ("page fault in cli mode");
	  }

      }

    if (PF::is_alignment_error(error_code))
      {
	printf("KERNEL: alignment error at %08lx (PC: %08lx)\n", pfa, pc);
        return false;
      }

    return current_thread()->handle_page_fault(pfa, error_code, pc, ret_frame);
  }

  void slowtrap_entry(Trap_state *ts)
  {
    Thread *t = current_thread();
    // send exception IPC if requested
    if (t->send_exception(ts))
      return;

    // exception handling failed
    if (Config::conservative)
      kdb_ke ("thread killed");

    t->halt();

  }

};

IMPLEMENT inline
bool
Thread::pagein_tcb_request(Return_frame *regs)
{
  if (*(Mword*)regs->pc == 0xe59ee000)
    {
      //printf("TCBR: %08lx\n", *(Mword*)regs->pc);
      // skip faulting instruction
      regs->pc += 4;
      // tell program that a pagefault occured we cannot handle
      regs->psr |= 0x40000000;	// set zero flag in psr
      regs->km_lr = 0;

      return true;
    }
  return false;
}

// ---------------------------------------------------------------------------
#include "std_macros.h"
#include "timer.h"
#include "dirq.h"
#include "pic.h"

extern "C"
{

  void irq_handler()
  {
    if (Pic::Multi_irq_pending)
      {
next_irq:
	Mword irqs = Pic::pending();
	if (Pic::is_pending(irqs, Config::Scheduling_irq))
	  {
	    Timer::acknowledge();
	    Timer::update_system_clock();
	    current_thread()->handle_timer_interrupt();
	    goto next_irq;
	  }

	for (unsigned irq=0; irq < sizeof(Mword)*8; irq++)
	  {
	    if (irqs & (1<<irq))
	      {
		Irq *i = Dirq::lookup(irq);
		Pic::block_locked(irq);
		nonull_static_cast<Dirq*>(i)->hit();
		goto next_irq;
	      }
	  }
      }
    else
      {
	Mword irq = Pic::pending();
	if (EXPECT_FALSE(!irq))
	  return;

	if (Pic::is_pending(irq, Config::Scheduling_irq))
	  {
	    Timer::acknowledge();
	    Timer::update_system_clock();
	    current_thread()->handle_timer_interrupt();
	  }
	else
	  {
	    Irq *i = Dirq::lookup(irq);
	    i->hit();
	  }
      }
  }

};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include "trap_state.h"


/** Constructor.
    @param space the address space
    @param id user-visible thread ID of the sender
    @param init_prio initial priority
    @param mcp thread's maximum controlled priority
    @post state() != Thread_invalid
 */
IMPLEMENT
Thread::Thread(Task* task,
               L4_uid id,
               unsigned short init_prio, unsigned short mcp)
  : Receiver (&_thread_lock,
              task,
              init_prio,
              mcp,
              Config::default_time_slice),
    Sender            (id),	// select optimized version of constructor
    _preemption       (id),
    _deadline_timeout (&_preemption)
{
  assert (current() == thread_lock()->lock_owner());
  assert (state() == Thread_invalid);

  if (Config::stack_depth)
    std::memset((char*)this + sizeof(Thread), '5',
                Config::thread_block_size-sizeof(Thread)-64);

  // set a magic value -- we use it later to verify the stack hasn't
  // been overrun
  _magic = magic;
  _task = task;
  _irq = 0;
  _recover_jmpbuf = 0;
  _timeout = 0;
  _exc_ip = ~0UL;
  _in_exception = false;

  *reinterpret_cast<void(**)()> (--_kernel_sp) = user_invoke;

  // clear out user regs that can be returned from the thread_ex_regs
  // system call to prevent covert channel
  Entry_frame *r = regs();
  r->sp(0);
  r->ip(0);

  mem_space()->kmem_update(this);
  // make sure the thread's kernel stack is mapped in its address space
  //  _task->kmem_update(reinterpret_cast<Address>(this));

  setup_utcb_kernel_addr();

  caps_init (current_thread());
  _pager = _ext_preempter = nil_thread;
  preemption()->set_receiver (nil_thread);

  present_enqueue();

  state_add(Thread_dead);
  
  // ok, we're ready to go!
}

IMPLEMENT inline
Mword
Thread::user_sp() const
{ return regs()->sp(); }

IMPLEMENT inline
void
Thread::user_sp(Mword sp)
{ return regs()->sp(sp); }

IMPLEMENT inline NEEDS[Thread::exception_triggered]
Mword
Thread::user_ip() const
{ return exception_triggered()?_exc_ip:regs()->ip(); }

IMPLEMENT inline
Mword
Thread::user_flags() const
{ return 0; }

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
      r->psr = (r->psr & ~Proc::Status_mode_mask) | Proc::Status_mode_user;
    }
}


PUBLIC inline NEEDS ["trap_state.h"]
int
Thread::send_exception(Trap_state *ts)
{
  if (_pager != kernel_thread)
    {
      state_change(~Thread_cancel, Thread_in_exception);

      Proc::sti();		// enable interrupts, we're sending IPC
      Ipc_err err = exception(ts);

      if (err.has_error() && err.error() == Ipc_err::Enot_existent)
	return 0;

      return 1;      // We did it
    }
  return 0;
}


PRIVATE inline
bool
Thread::invalid_ipc_buffer(void const *a)
{
  if (!_in_exception)
    return Mem_layout::in_kernel(((Address)a & Config::SUPERPAGE_MASK)
                                 + Config::SUPERPAGE_SIZE - 1);

  return false;
}

PRIVATE inline
int
Thread::do_trigger_exception(Entry_frame *r)
{
  if (_exc_ip == ~0UL)
    {
      Lock_guard<Cpu_lock> lock(&cpu_lock);
      _exc_ip = r->ip();
      
      extern char leave_by_trigger_exception[];
      r->pc = (Mword)leave_by_trigger_exception;
      r->psr &= ~Proc::Status_mode_mask; // clear mode
      r->psr |= Proc::Status_mode_supervisor
	| Proc::Status_interrupts_disabled;

      return 1;
    }
  return 0;
}


PUBLIC inline
void
Thread::transfer_fpu(Thread *)
{}


PRIVATE static inline
void
Thread::copy_utcb_to_ts(L4_msg_tag const &tag, Thread *snd, Thread *rcv)
{
  Trap_state *ts = (Trap_state*)rcv->_utcb_handler;
  Utcb *snd_utcb = (Utcb*)snd->local_id();
  Mword       s  = tag.words();

  if (EXPECT_FALSE(rcv->exception_triggered()))
    {
      // triggered exception pending
      Cpu::memcpy_mwords (ts, snd_utcb->values, s > 19 ? 19 : s);
      if (EXPECT_TRUE(s > 19))
	rcv->user_ip(snd_utcb->values[19]);

      ts->cpsr &= ~Proc::Status_mode_mask; // clear mode
      ts->cpsr |= Proc::Status_mode_supervisor
	| Proc::Status_interrupts_disabled;
    }
  else
    {
      Cpu::memcpy_mwords (ts, snd_utcb->values, s > 20 ? 20 : s);
      ts->cpsr &= ~Proc::Status_mode_mask; // clear mode
      ts->cpsr |= Proc::Status_mode_user;
    }

  if (tag.transfer_fpu())
    snd->transfer_fpu(rcv);

  rcv->state_del(Thread_in_exception);
}


PRIVATE static inline NEEDS[Thread::access_utcb]
void
Thread::copy_ts_to_utcb(L4_msg_tag const &, Thread *snd, Thread *rcv)
{
  Trap_state *ts = (Trap_state*)snd->_utcb_handler;

  {
    Lock_guard <Cpu_lock> guard (&cpu_lock);
    Utcb *rcv_utcb = rcv->access_utcb();

    if (EXPECT_FALSE(snd->exception_triggered()))
      {
	Cpu::memcpy_mwords (rcv_utcb->values, ts, 19);
	rcv_utcb->values[19] = snd->user_ip();
      }
    else
      Cpu::memcpy_mwords (rcv_utcb->values, ts, 20);

    if (rcv_utcb->inherit_fpu())
	snd->transfer_fpu(rcv);

  }
}


//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && vcache]:

PRIVATE inline
Utcb*
Thread::access_utcb() const
{
  // Do not use the alias mapping of the UTCB for the current address space
  return current_mem_space() == mem_space() 
    ? (Utcb*)local_id()
    : utcb();
}


//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && !vcache]:

PRIVATE inline
Utcb*
Thread::access_utcb() const
{
  return utcb();
}
