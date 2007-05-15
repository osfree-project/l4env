INTERFACE:

#include "sender.h"
#include "irq_alloc.h"

/** Hardware interrupts.  This class encapsulates handware IRQs.  Also,
    it provides a registry that ensures that only one receiver can sign up
    to receive interrupt IPC messages.
 */
class Irq : public Irq_alloc, public Sender
{
protected:
  Irq(); // default constructors remain undefined

private:
  Irq(Irq&);

protected:
  Smword _queued;

public:
  virtual void mask() = 0;
  virtual void mask_and_ack() = 0;
  virtual bool check_debug_irq() = 0;

  static Irq *lookup( unsigned irq );

};

extern "C" void fast_ret_from_irq(void);

IMPLEMENTATION:

#include "atomic.h"
#include "config.h"
#include "cpu_lock.h"
#include "entry_frame.h"
#include "globals.h"
#include "kdb_ke.h"
#include "lock_guard.h"
#include "receiver.h"
#include "std_macros.h"
#include "thread_lock.h"
#include "thread_state.h"

/** Bind a receiver to this device interrupt.
    @param t the receiver that wants to receive IPC messages for this IRQ
    @return true if the binding could be established
 */
PUBLIC inline NEEDS ["atomic.h", "cpu_lock.h", "lock_guard.h"]
bool 
Irq::alloc(Receiver *t)
{
  bool ret = cas (&_irq_thread, reinterpret_cast<Receiver*>(0), t);

  if (ret) 
    _queued = 0;

  return ret;
}


/** Release an device interrupt.
    @param t the receiver that ownes the IRQ
    @return true if t really was the owner of the IRQ and operation was 
            successful
 */
PUBLIC
bool
Irq::free(Receiver *t)
{
  bool ret = cas (&_irq_thread, t, reinterpret_cast<Receiver*>(0));

  if (ret) 
    {
	{
	  Lock_guard<Cpu_lock> guard(&cpu_lock);
	  mask();
	}
      sender_dequeue(t->sender_list());
    }

  return ret;
}


IMPLEMENT inline
Irq *Irq::lookup( unsigned irq )
{
  return static_cast<Irq*>(Irq_alloc::lookup(irq));
}


/** Constructor.
    @param irqnum number of hardware interrupt this instance will be bound to
 */
PUBLIC
explicit
Irq::Irq(unsigned irqnum)
  : Irq_alloc (irqnum), Sender (Global_id::irq (irqnum)), _queued (0)
{}


/** Consume one interrupt.
    @return number of IRQs that are still pending.
 */
PRIVATE inline NEEDS ["atomic.h"]
Smword
Irq::consume ()
{
  Smword old;

  do 
    {
      old = _queued;
    }
  while (!cas (&_queued, old, old - 1));

  return old - 1;
}

PUBLIC inline
int
Irq::queued ()
{
  return _queued;
}

PUBLIC inline NEEDS["config.h","entry_frame.h","globals.h","kdb_ke.h",
		    "receiver.h","thread_lock.h","thread_state.h"]
void
Irq::hit()
{
  // We're entered holding the kernel lock, which also means irqs are
  // disabled on this CPU (XXX always correct?).  We never enable irqs
  // in this stack frame (except maybe in a nonnested invocation of
  // switch_exec() -> switchin_context()) -- they will be re-enabled
  // once we return from it (iret in entry.S:all_irqs) or we switch to
  // a different thread.

  assert (cpu_lock.test());

  mask_and_ack();

  if (EXPECT_FALSE (!_irq_thread))
    return;
  else if (EXPECT_FALSE (_irq_thread == (void*)-1))
    {
      // debugger attached to IRQ
#if defined(CONFIG_KDB) || defined(CONFIG_JDB)
      if (check_debug_irq())
        kdb_ke("IRQ ENTRY");
#endif
      unmask();
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
