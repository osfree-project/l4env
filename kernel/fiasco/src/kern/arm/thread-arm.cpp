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

IMPLEMENT inline void Thread::rcv_startup_msg() {} // dummy

IMPLEMENT
void
Thread::user_invoke()
{
  //  printf("Thread: %p [state=%08x, space=%p]\n", current(), current()->state(), current_space() );

  assert (current()->state() & Thread_ready);
#if 0
  printf("user_invoke of %p @ %08x sp=%08x\n",
	 current(),current()->regs()->ip(),
	 current()->regs()->sp() );
  //current()->regs()->sp(0x30000);
#endif

  register unsigned long r0 asm ("r0")
    = Kmem::virt_to_phys(Kip::k()).get_unsigned();

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
  if(pfa<Kmem::Sdram_phys_base && pfa>=Kmem::Sdram_phys_base + 256*1024*1024)
    {
      printf("BAD Sigma0 access @%08lx\n", pfa);
      return false;
    }
#if 0
  if(pfa>=0x80000000)
    { // adapter area
      printf("MAP adapter area: %08x\n",pfa & Config::SUPERPAGE_MASK);
      return (space()->v_insert((pfa & Config::SUPERPAGE_MASK),
				(pfa & Config::SUPERPAGE_MASK),
				Config::SUPERPAGE_SIZE,
				Space::Page_writable
				| Space::Page_user_accessible
				| Space::Page_noncacheable)
	      != Space::Insert_err_nomem);
    }
  else
#endif
    {
      return (space()->v_insert((pfa & Config::SUPERPAGE_MASK),
				(pfa & Config::SUPERPAGE_MASK),
				Config::SUPERPAGE_SIZE,
				Space::Page_writable
				| Space::Page_user_accessible)
	      != Space::Insert_err_nomem);
    }
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
                        const Mword pc, Mword *const sp)
  {
    // Pagefault in user mode or interrupts were enabled
    if (PF::is_usermode_error(error_code) || !(sp[5] /*spsr*/ & 128))
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
	    if (Thread::pagein_tcb_request(pc))
	      {
		// skip faulting instruction
		sp[9] += 4;
		// tell program that a pagefault occured we cannot handle
		sp[5] |= 0x40000000;	// set zero flag in psr

		return true;
	      }
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
      return false;

    return current_thread()->handle_page_fault(pfa, error_code, pc);
  }

  void slowtrap_entry(Trap_state *ts)
  {
    Thread *t = current_thread();
    // send exception IPC if requested
    if (t->snd_exception(ts))
      return;


    // exception handling failed
    if (Config::conservative)
      kdb_ke ("thread killed");

    t->halt();

  }

};

IMPLEMENT inline NOEXPORT
Mword
Thread::pagein_tcb_request(Address pc)
{
  return *(Mword*)pc == 0xe59ee000 && *(Mword*)(pc + 4) == 0x03a0e000;
}

IMPLEMENT inline
Mword
Thread::is_tcb_mapped() const
{
  register Mword pagefault_if_0 asm("r14");
  asm volatile ("msr cpsr_f, #0 \n" // clear flags
                "ldr %0, [%0]   \n"
		"moveq %0, #0   \n"
		: "=r" (pagefault_if_0) : "0" (&_state));
  return pagefault_if_0;
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
	    Pic::block_locked(irq);
	    nonull_static_cast<Dirq*>(i)->hit();
	  }
      }
  }

};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm-x0]:

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
    _deadline_timeout (&_preemption),
    _activation	      (id)
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
  Lock_guard <Thread_lock> guard (thread_lock());

  *reinterpret_cast<void(**)()> (--_kernel_sp) = user_invoke;

  // clear out user regs that can be returned from the thread_ex_regs
  // system call to prevent covert channel
  Entry_frame *r = regs();
  r->sp(0);
  r->ip(0);

  _task->kmem_update(this);
  // make sure the thread's kernel stack is mapped in its address space
  //  _task->kmem_update(reinterpret_cast<Address>(this));

  _pager = _ext_preempter = nil_thread;
  preemption()->set_receiver (nil_thread);

  if (space_index() == Thread::lookup(thread_lock()->lock_owner())
                       ->space_index())
    {
      // same task -> enqueue after creator
      present_enqueue(Thread::lookup(thread_lock()->lock_owner()));
    }
  else
    {
      // other task -> enqueue in front of this task
      present_enqueue(lookup_first_thread(Thread::lookup
					  (thread_lock()->lock_owner())
					  ->space_index())
                      ->present_prev);
      // that's safe because thread 0 of a task is always present
    }

  state_add(Thread_dead);

  // ok, we're ready to go!
}

/** (Re-) Ininialize a thread and make it ready.
    This call also cancels IPC.
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
		   Address *o_ip = 0,
		   Address *o_sp = 0,
		   Thread **o_pager = 0,
		   Receiver **o_preempter = 0,
		   Address *o_eflags = 0,
		   bool no_cancel = 0,
		   bool alien = 0)
{
  (void)o_eflags;

  assert (current() == thread_lock()->lock_owner());

  if (state() == Thread_invalid)
    return false;

  Entry_frame *r = regs();

  if (o_pager) *o_pager = _pager;
  if (o_preempter) *o_preempter = preemption()->receiver();
  if (o_sp) *o_sp = r->sp();
  if (o_ip) *o_ip = r->ip();
  //if (o_eflags) *o_eflags = r->eflags;

  if (ip != 0xffffffff)
    {
      r->ip(ip);
      r->psr = (r->psr & ~0x01f) | 0x10;
      
      if (alien)
	state_change (~0, Thread_alien);
      else
	state_change (~Thread_alien, 0);

      if (! (state() & Thread_dead))
	{
#if 0
	  kdb_ke("reseting non-dead thread");
#endif
	  if (!no_cancel)
	  // cancel ongoing IPC or other activity
	    state_change(~Thread_ipc_in_progress,
                         Thread_cancel | Thread_ready);
	}
      else
        state_change(~Thread_dead, Thread_ready);
    }

  if (pager != 0) _pager = pager;
  if (preempter != 0) preemption()->set_receiver (preempter);
  if (sp != 0xffffffff) r->sp( sp );

  return true;
}

PUBLIC inline NEEDS ["trap_state.h"]
Mword
Thread::snd_exception(Trap_state *ts)
{
  state_del(Thread_cancel);
  Proc::sti();		// enable interrupts, we're sending IPC
  exception(ts);
  return 1;      // We did it
}

PRIVATE
Ipc_err
Thread::exception(Trap_state *ts)
{
  if (space()->is_sigma0())
    {
      puts("Exception in Sigma0: KILL SIGMA0");
      ts->dump();
      halt();
    }

  Sys_ipc_frame r;
  struct Message_header
    {
      L4_fpage     fp;
      L4_msgdope   size_dope;
      L4_msgdope   snd_dope;
      Mword        words[3];
      L4_str_dope  excp_regs;
    };

  Message_header snd_msg;


  L4_timeout timeout( L4_timeout::Never );
  Thread *handler = _pager; // _exception_handler;

  // fill registers for IPC
  r.set_msg_word(0, L4_exception_ipc::Exception_ipc_cookie_1);
  r.set_msg_word(1, L4_exception_ipc::Exception_ipc_cookie_2);
  r.set_msg_word(2, 0); // nop in V2
  r.snd_desc((Mword)&snd_msg);
  r.rcv_desc((Mword)&snd_msg);
  snd_msg.fp = L4_fpage();
  snd_msg.size_dope = L4_msgdope(3,1);
  snd_msg.snd_dope  = L4_msgdope(3,1);
  snd_msg.excp_regs.snd_size = sizeof(Trap_state);
  snd_msg.excp_regs.rcv_size = sizeof(Trap_state);
  snd_msg.excp_regs.snd_str = (Unsigned8*)ts;
  snd_msg.excp_regs.rcv_str = (Unsigned8*)ts;

  prepare_receive(handler, &r);

  _in_exception = true;

  Ipc_err err = do_send(handler, timeout, &r);
  Ipc_err ret (0);

  if (EXPECT_FALSE(err.has_error()))
    {
      if (err.error() == Ipc_err::Semsgcut
	  || err.error() == Ipc_err::Sercvpfto)
	{
	  printf("Pager is not willing to handle exceptions\n"
	         "KILL Thread: ");
	  id().print();
	  puts("");
	  ts->dump();

	  halt();
	}

      if (Config::conservative)
	{
	  printf("exception send error = 0x%lx\n"
	         "KILL thread\n", err.raw());
	  ts->dump();
	  halt();
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
	      printf("exception rcv error = 0x%lx\n"
		     "KILL Thread\n", err.raw());
	      ts->dump();
	      halt();
	      kdb_ke("rcv from pager failed");
	    }

	}

      if (r.msg_word(0) == 1)
	state_add(Thread_dis_alien);

      // enforce user mode;
      ts->cpsr &= ~0x1f;
      ts->cpsr |= 0x10;
      // else ret = 0 -- that's the default, and it means: retry
    }

  _in_exception = false;
  return ret;
}

PRIVATE inline
bool
Thread::invalid_ipc_buffer(void const *a)
{
  if (!_in_exception)
    return
      Mem_layout::in_kernel(((Address)a & Config::SUPERPAGE_MASK)
	      + Config::SUPERPAGE_SIZE - 1);

  return false;
}
