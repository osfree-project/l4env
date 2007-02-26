INTERFACE:

#include "irq.h"
#include "initcalls.h"
#include "types.h"

class Receiver;

class Dirq : public Irq
{
public:
  static void init() FIASCO_INIT;
  void *operator new (size_t);

  bool alloc(Receiver *t, bool ack_in_kernel);
  bool free(Receiver *t);

private:
  Dirq();
  Dirq(Dirq&);
};


IMPLEMENTATION:

#include "config.h"
#include "entry_frame.h"
#include "globals.h"
#include "initcalls.h"
#include "kdb_ke.h"
#include "pic.h"
#include "std_macros.h"
#include "receiver.h"
#include "thread_lock.h"
#include "thread_state.h"
#include "vkey.h"

static char dirq_storage[sizeof(Dirq) * Config::Max_num_dirqs];

IMPLEMENT 
void *Dirq::operator new (size_t) 
{
  static unsigned first = 0;
  assert(first < Config::Max_num_dirqs);
  return reinterpret_cast<Dirq*>(dirq_storage)+(first++);
}

IMPLEMENT FIASCO_INIT
void
Dirq::init()
{
  for( unsigned i = 0; i<Config::Max_num_dirqs; ++i )
    new Dirq(i);
}


PUBLIC inline
explicit
Dirq::Dirq(unsigned irqnum) : Irq(irqnum)
{}


PUBLIC inline NEEDS["config.h","entry_frame.h","globals.h","kdb_ke.h",
		    "pic.h","receiver.h","thread_lock.h","thread_state.h",
		    "vkey.h"]
void
Dirq::hit()
{
  // We're entered holding the kernel lock, which also means irqs are
  // disabled on this CPU (XXX always correct?).  We never enable irqs
  // in this stack frame (except maybe in a nonnested invocation of
  // switch_exec() -> switchin_context()) -- they will be re-enabled
  // once we return from it (iret in entry.S:all_irqs) or we switch to
  // a different thread.

  if (EXPECT_FALSE (!_irq_thread))
    Pic::disable_locked(id().irq());

  else if (EXPECT_FALSE (_irq_thread == (void*)-1))
    {
      // debugger attached to IRQ
#if defined(CONFIG_KDB) || defined(CONFIG_JDB)
      if (!Vkey::check_(id().irq()))
        kdb_ke("IRQ ENTRY");
#endif
      Pic::enable_locked(id().irq(), ~0U);
      return;
    }

  else if (EXPECT_TRUE (_queued++ == 0))	// increase hit counter
    {
      set_receiver (_irq_thread);
      if (!Config::Irq_shortcut)
	{
	  // in profile mode, don't optimize
	  // in non-profile mode, enqueue _after_ shortcut if still necessary
	  sender_enqueue(_irq_thread->sender_list(), 255);
	}
 
      // if the thread is waiting for this interrupt, make it ready;
      // this will cause it to run irq->receiver_ready(), which
      // handles the rest

      // XXX careful!  This code may run in midst of an do_ipc()
      // operation (or similar)!

      if (_irq_thread->sender_ok (this))
	{
	  if (EXPECT_TRUE
	      (current() != _irq_thread
               && Context::can_preempt_current (_irq_thread->sched())
	       // avoid race in do_ipc() after Thread_send_in_progress
	       // flag was deleted from _irq_thread's thread state
	       && !(_irq_thread->state() & 
		               (Thread_ready | Thread_delayed_deadline))
	       && !_irq_thread->thread_lock()->test() // irq_thread not locked?
	       && !Context::schedule_in_progress())) // no schedule in progress
	    {
    	      // we don't need to manipulate the state in a safe way
    	      // because we are still running with interrupts turned off
    	      _irq_thread->state_change_dirty(~Thread_busy, Thread_ready);

	      if (!Config::Irq_shortcut)
		{
		  // no shortcut: switch to the interrupt thread which will
		  // calls Irq::ipc_receiver_ready
		  current()->switch_to_locked (_irq_thread);
		  return;
		}

	      // The following shortcut optimization does not work if PROFILE
	      // is defined because fast_ret_from_irq does not handle the
	      // different implementation of the kernel lock in profiling mode

	      // At this point we are sure that the connected interrupt
    	      // thread is waiting for the next interrupt and that its 
	      // thread priority is higher than the current one. So we
	      // choose a short cut: Instead of doing the full ipc handshake
	      // we simply build up the return stack frame and go out as 
	      // quick as possible.
	      // 
	      // XXX We must own the kernel lock for this optimization!
	      //

    	      Sys_ipc_frame* dst_regs = _irq_thread->rcv_regs();
	      Mword *esp = reinterpret_cast<Mword*>(dst_regs);

	      // set return address of irq_thread
	      *--esp = reinterpret_cast<Mword>(fast_ret_from_irq);

	      // XXX set stack pointer of irq_thread
	      _irq_thread->set_kernel_sp(esp);

	      // set ipc return value: OK
	      dst_regs->msg_dope(0);

	      // set ipc source thread id
	      dst_regs->rcv_src(id());

	      assert(_queued == 1);
	      _queued = 0;

	      // Now that the interrupt has been delivered, it is OK for it to
	      // occur again.
	      maybe_enable();

	      // ipc completed
    	      _irq_thread->state_change_dirty(~Thread_ipc_mask, 0);

	      // in case a timeout was set
	      _irq_thread->reset_timeout();

	      // directly switch to the interrupt thread context and go out
	      // fast using fast_ret_from_irq (implemented in assembler).
	      // kernel-unlock is done in switch_exec() (on switchee's side).

	      // no shortcut if profiling: switch to the interrupt thread
	      current()->switch_to_locked (_irq_thread);
	      return;
	    }
	  // we don't need to manipulate the state in a safe way
      	  // because we are still running with interrupts turned off
	  _irq_thread->state_change_dirty(~Thread_busy, Thread_ready);
	  _irq_thread->ready_enqueue();
	}

      if (Config::Irq_shortcut)
	// in profile mode, don't optimize
	// in non-profile mode, enqueue after shortcut if still necessary
	sender_enqueue(_irq_thread->sender_list(), 255);
    }
}

