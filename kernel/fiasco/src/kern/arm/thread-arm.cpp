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

PUBLIC inline
void
Thread::destroy_utcb()
{}

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
	Mem_space::Page_writable | Mem_space::Page_user_accessible)
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
                        const Mword pc, Mword *const sp)
  {
    // Pagefault in user mode
    if (PF::is_usermode_error(error_code))
      {
	current_thread()->state_del(Thread_cancel);
        Proc::sti();
      }
    // or interrupts were enabled
    else if (!(sp[5] /*spsr*/ & Proc::Status_IRQ_disabled))
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

    return current_thread()->handle_page_fault(pfa, error_code, pc, 
	(Return_frame*)(sp+5));
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
	    i->maybe_acknowledge();
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

  Lock_guard <Thread_lock> guard (thread_lock());

  *reinterpret_cast<void(**)()> (--_kernel_sp) = user_invoke;

  // clear out user regs that can be returned from the thread_ex_regs
  // system call to prevent covert channel
  Entry_frame *r = regs();
  r->sp(0);
  r->ip(0);

  mem_space()->kmem_update(this);
  // make sure the thread's kernel stack is mapped in its address space
  //  _task->kmem_update(reinterpret_cast<Address>(this));

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
{ return exception_triggered()?(_exc_ip & ~0x03):regs()->ip(); }

IMPLEMENT inline
Mword
Thread::user_flags() const
{ return 0; }

IMPLEMENT inline NEEDS[Thread::exception_triggered]
void
Thread::user_ip(Mword ip)
{ 
  if (exception_triggered())
    _exc_ip = (_exc_ip & 0x03) | ip;
  else
    {
      Entry_frame *r = regs();
      r->ip(ip);
      r->psr = (r->psr & ~Proc::Status_mode_mask) | Proc::Status_mode_user;
    }
}


PUBLIC inline NEEDS ["trap_state.h"]
Mword
Thread::send_exception(Trap_state *ts)
{
  state_del(Thread_cancel);
  Proc::sti();        // enable interrupts, we're sending IPC

  Ipc_err err = exception(ts);

  if (err.has_error() && err.error() == Ipc_err::Enot_existent)
    return 0;

  return 1;           // We did it
}


PRIVATE
Ipc_err
Thread::exception(Trap_state *ts)
{
  if (mem_space()->is_sigma0())
    {
      puts("KERNEL: Exception in Sigma0: killing SIGMA0");
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

  if (! revalidate(handler))
    {
      WARN ("Denying %x.%x to send exception message (pc=%x"
	    ", error=%x) to %x.%x",
	    id().task(), id().lthread(), ts->pc, ts->error_code, 
	    handler ? handler->id().task() : L4_uid (L4_uid::Invalid).task(),
	    handler ? handler->id().lthread() 
	            : L4_uid (L4_uid::Invalid).lthread());

      return Ipc_err(Ipc_err::Enot_existent);
    }

  // fill registers for IPC
  r.set_msg_word(0, L4_exception_ipc::Exception_ipc_cookie_1);
  r.set_msg_word(1, L4_exception_ipc::Exception_ipc_cookie_2);
  r.set_msg_word(2, 0); // nop in V2
  r.snd_desc((Mword)&snd_msg);
  r.rcv_desc((Mword)&snd_msg);
  snd_msg.fp = L4_fpage::all_spaces();
  snd_msg.size_dope = L4_msgdope(L4_snd_desc(0), 3,1);
  snd_msg.snd_dope  = L4_msgdope(L4_snd_desc(0), 3,1);
  snd_msg.excp_regs.snd_size = sizeof(Trap_state);
  snd_msg.excp_regs.rcv_size = sizeof(Trap_state);
  snd_msg.excp_regs.snd_str = (Unsigned8*)ts;
  snd_msg.excp_regs.rcv_str = (Unsigned8*)ts;

  _in_exception = true;
  asm volatile ( "" : : : "memory" );

  if (_exc_ip != ~0UL)
    ts->pc = user_ip();

  Ipc_err ret (0);
  Proc::cli();
  Ipc_err err = do_ipc(true, handler,
                       true, handler,
                       timeout, &r);
  Proc::sti();

  if (EXPECT_FALSE(err.has_error()
                   && err.error() != Ipc_err::Recanceled
                   && err.error() != Ipc_err::Secanceled
                   && err.error() != Ipc_err::Seaborted
                   && err.error() != Ipc_err::Reaborted))
    {
      if (Config::conservative)
        {
          printf("KERNEL: Error %s exception (err=0x%lx) %s ",
                 err.snd_error() ? "sending" : "rcveiving", err.raw(),
                 err.snd_error() ? "to" : "from");
          handler->id().print();
          printf("\nKERNEL: killing thread ");
          id().print();
          puts("");
          ts->dump();
          halt();

          if (err.snd_error())
            kdb_ke("snd to pager failed");
          else
            kdb_ke("rcv from pager failed");
        }

      if (err.snd_error())
        {
          if (err.error() == Ipc_err::Enot_existent)
            ret = (state() & Thread_cancel)
              ? Ipc_err (0)
              : err;
          else if (err.error() == Ipc_err::Semsgcut
                   || err.error() == Ipc_err::Sercvpfto)
            {
              printf("KERNEL: Pager is not willing to handle exceptions\n"
                     "KERNEL: killing thread: ");
              id().print();
              puts("");
              ts->dump();

              halt();
            }
        }
    }


  if (r.msg_word(0) == 1)
    state_add(Thread_dis_alien);

  {
    Lock_guard<Cpu_lock> lock(&cpu_lock);

    if (EXPECT_FALSE(_exc_ip != ~0UL))
      {
        extern char leave_by_trigger_exception[];
	user_ip(ts->pc);
        ts->pc = (Mword)leave_by_trigger_exception;
        ts->cpsr &= ~Proc::Status_mode_mask; // clear mode
        ts->cpsr |= Proc::Status_mode_supervisor
                     | Proc::Status_interrupts_disabled;
      }
    else
      {
        ts->cpsr &= ~Proc::Status_mode_mask; // clear mode
        ts->cpsr |= Proc::Status_mode_user;
      }

    _in_exception = false;
  }

  return ret;
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
      // store FIQ and IRQ Mask bits in lowest two bits of exception IP
      // (PC ist always word aligned --> no Thumb support yet)
      _exc_ip |= (r->psr >> 6) & 0x03; 
      if (!_in_exception)
        {
          extern char leave_by_trigger_exception[];
          r->pc = (Mword)leave_by_trigger_exception;
          r->psr &= ~Proc::Status_mode_mask; // clear mode
          r->psr |= Proc::Status_mode_supervisor
                     | Proc::Status_interrupts_disabled;
        }
      return 1;
    }
  return 0;
}
