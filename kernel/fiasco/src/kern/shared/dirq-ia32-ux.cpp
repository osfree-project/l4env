
IMPLEMENTATION[ia32-ux]:

#include <cassert>

#include "atomic.h"
#include "cpu_lock.h"
#include "entry_frame.h"
#include "globalconfig.h"
#include "globals.h"
#include "initcalls.h"
#include "kdb_ke.h"
#include "logdefs.h"
#include "pic.h"
#include "receiver.h"
#include "static_init.h"
#include "std_macros.h"
#include "thread_state.h"


#ifdef CONFIG_APIC_MASK
extern "C" unsigned apic_timer_entry;
extern "C" unsigned apic_irq_mask;
extern "C" unsigned apic_irq_nr;
#endif


static char dirq_storage[sizeof(Dirq)* 16]; // 16 device irq's

IMPLEMENT 
void *Dirq::operator new ( size_t ) 
{
  static unsigned first = 0;
  return reinterpret_cast<Dirq*>(dirq_storage)+(first++);
}

IMPLEMENT FIASCO_INIT
void
Dirq::init()
{
  for( unsigned i = 0; i<16; ++i )
    new Dirq(i);
}


/** Bind a receiver to this device interrupt.
    @param t the receiver that wants to receive IPC messages for this IRQ
    @param ack_in_kernel if true, the kernel will acknowledge the interrupt,
                         as opposed to user-lever acknowledgement
    @return true if the binding could be established
 */
IMPLEMENT inline NEEDS ["atomic.h","pic.h"]
bool Dirq::alloc(Receiver *t, bool ack_in_kernel)
{
  bool ret = smp_cas(&_irq_thread, reinterpret_cast<Receiver*>(0), t);

  if (ret) 
    {
      int irq = id().irq();

      _ack_in_kernel = ack_in_kernel;
      _queued = 0;

      if (_ack_in_kernel)
	Pic::enable(irq);
      else
	Pic::disable(irq);
    }

  return ret;
}

/** Release an device interrupt.
    @param t the receiver that ownes the IRQ
    @return true if t really was the owner of the IRQ and operation was 
            successful
 */
IMPLEMENT inline NEEDS ["receiver.h","pic.h"]
bool Dirq::free(Receiver *t)
{
  bool ret = smp_cas(&_irq_thread, t, reinterpret_cast<Receiver*>(0));

  if (ret) 
    {
      Pic::disable(id().irq());
      sender_dequeue(t->sender_list());
    }

  return ret;
}

/** The corresponding hardware interrupt occurred -- handle it. 
    This method checks whether the attached receiver is ready to receive 
    an IRQ message, and if so, restarts it so that it can pick up the IRQ
    message using ipc_receiver_ready().
 */
PUBLIC inline NOEXPORT
void Dirq::hit()
{
  // We're entered holding the kernel lock, which also means irqs are
  // disabled on this CPU (XXX always correct?).  We never enable irqs
  // in this stack frame (except maybe in a nonnested invocation of
  // switch_to() -> switchin_context()) -- they will be re-enabled
  // once we return from it (iret in entry.S:all_irqs) or we switch to
  // a different thread.

  if (EXPECT_FALSE (!_irq_thread) )
    {
      Pic::disable_locked(id().irq());
    }
  else if (EXPECT_FALSE (_irq_thread == (void*)-1))
    {
      /* debugger attached to IRQ */
#if defined(CONFIG_JDB)
      kdb_ke("IRQ ENTRY");
#endif
      Pic::enable_locked(id().irq());
      return;
    }
  else if (_queued++ == 0)	// increase hit counter
    {
      set_receiver (_irq_thread);
#if !defined(CONFIG_ASSEMBLER_IPC_SHORTCUT) || defined(CONFIG_PROFILE)
      // in profile mode, don't optimize
      // in non-profile mode, enqueue after shortcut if still necessary
      sender_enqueue(_irq_thread->sender_list());
#endif
      
      // if the thread is waiting for this interrupt, make it running;
      // this will cause it to run irq->receiver_ready(), which
      // handles the rest

      // XXX careful!  This code may run in midst of an ipc_send_regs
      // operation (or similar)!

      if (_irq_thread->sender_ok (this))
	{
	  if (current() != _irq_thread
	      && _irq_thread->sched()->prio() >= current()->sched()->prio()
	      // avoid race in ipc_send_regs() after Thread_send_in_progress
	      // flag was deleted from _irq_thread's thread state
	      && !(_irq_thread->state() & Thread_running))
	    {
    	      // we don't need to manipulate the state in a safe way
    	      // because we are still running with interrupts turned off
    	      _irq_thread->state_change_dirty(~Thread_busy, Thread_running);

	      // the following shortcut optimization does not work if PROFILE
	      // is defined because fast_ret_from_irq does not handle the
	      // different implementation of the kernel lock in profiling mode
#if defined(CONFIG_ASSEMBLER_IPC_SHORTCUT) && !defined(CONFIG_PROFILE)

	      // At this point we are sure that the connected interrupt
	      // thread is waiting for the next interrupt and that its 
	      // thread priority is higher than the current one. So we
	      // choose a short cut: Instead of doing the full ipc handshake
	      // we simply build up the return stack frame and go out as 
	      // quick as possible.
	      // 
	      // XXX We must own the kernel lock for this optimization!

	      Sys_ipc_frame* dest_regs = _irq_thread->receive_regs();
	      Mword *esp = reinterpret_cast<Mword*>(dest_regs);

	      // set return address of irq_thread
	      *--esp = reinterpret_cast<Mword>(fast_ret_from_irq);

	      // XXX set stack pointer of irq_thread
	      _irq_thread->set_kernel_sp(esp);

	      // set ipc return value: OK
	      dest_regs->msg_dope(0);
	      // set ipc source thread id
	      dest_regs->rcv_source( id() );

#ifdef CONFIG_APIC_MASK
	      if (id().irq() == apic_irq_nr)
		{
		  dest_regs->set_msg_word(0, apic_timer_entry);
		  dest_regs->set_msg_word(1, apic_irq_mask);
		}
#endif

	      assert(_queued == 1);
	      _queued = 0;

	      // Now that the interrupt has been delivered, it is OK for it to
	      // occur again.
	      maybe_enable();

	      // ipc completed
	      _irq_thread->state_change_dirty(~Thread_ipc_mask, 0);

	      // in case a timeout was set
	      _irq_thread->reset_timeout();

	      // kernel-unlock is done in switch_to() (on switchee's side)
	      
	      // directly switch to the interrupt thread context and go out 
	      // fast using fast_ret_from_irq (implemented in assembler)
#else
	      // no shortcut if profiling: switch to the interrupt thread
#endif
	      current()->switch_to(_irq_thread);
	  
	      return;
	    }
	  else
	    {
	      // we don't need to manipulate the state in a safe way
	      // because we are still running with interrupts turned off
	      _irq_thread->state_change_dirty(~Thread_busy, Thread_running);

	      _irq_thread->ready_enqueue();
	    }
	}

#ifndef CONFIG_PROFILE
      // in profile mode, don't optimize
      // in non-profile mode, enqueue after shortcut if still necessary
      sender_enqueue(_irq_thread->sender_list());
#endif
    }
}

/** Hardware interrupt entry point.  Calls corresponding Dirq instance's
    Dirq::hit() method.
    @param irqnum hardware-interrupt number
 */
extern "C" void
irq_interrupt(unsigned irqnum)
{
  LOG_IRQ(irqnum);

  // we're entered with disabled irqs
  Irq *i = Dirq::lookup(irqnum);

  i->maybe_acknowledge();

#if 0				// screen spinner for debugging purposes
  (*(unsigned char*)(kmem::phys_to_virt(0xb8000 + irqnum*2)))++;
#endif

#if defined(CONFIG_IA32) && defined(CONFIG_PROFILE)
  cpu_lock.lock();
#endif

  static_cast<Dirq*>(i)->hit();

#if defined(CONFIG_IA32) && defined(CONFIG_PROFILE)
  cpu_lock.clear_irqdisable(); 
#endif
}

