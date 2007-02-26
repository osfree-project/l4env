INTERFACE:

#include <setjmp.h>             // typedef jmp_buf

#include "l4_types.h"

#include "config.h"
#include "preemption.h"
#include "receiver.h"
#include "sched_timeout.h"
#include "sender.h"
#include "space.h"		// Space_index
#include "thread_lock.h"
#include "threadid.h"

class Irq_alloc;
class Return_frame;
class Syscall_frame;

/** A thread.  This class is the driver class for most kernel functionality.
 */
class Thread : public Receiver, public Sender
{
  friend class Jdb;
  friend class Jdb_bt;
  friend class Jdb_tcb;
  friend class Jdb_thread_list;
  friend class Jdb_list_threads;

public:
  void sys_ipc();
  void sys_fpage_unmap();
  void sys_thread_switch();
  void sys_thread_schedule();
  void sys_task_new();

  template < typename T > 
  void copy_from_user( T *kdst, T const *usrc, size_t n );
  template < typename T > 
  void copy_to_user  ( T *udst, T const *ksrc, size_t n );
  template < typename T > 
  T peek_user( T const *addr );
  template < typename T > 
  void poke_user( T *addr, T value );

  bool handle_page_fault( Address pfa, Mword error, Mword pc );

  static void handle_timer_interrupt();

private:
  Thread(const Thread&);	///< Default copy constructor is undefined
  void *operator new(size_t);	///< Default new operator undefined

  void preemption_event(Sched_context *sched);
  bool handle_sigma0_page_fault( Address pfa );
  bool handle_smas_page_fault( Address pfa, Mword error,
			       L4_msgdope &ipc_code );

  void kill_small_space();
  Mword small_space();
  void set_small_space(Mword nr);
  
protected:
  // implementation details follow...

  // DATA

  // Preemption IPC sender role
  Preemption _preemption;
  
  // Scheduler Timeout
  Sched_timeout _sched_timeout;

  // Another critical TCB cache line:
  Space*       _space;
  Thread_lock  _thread_lock;

  // More ipc state
  Thread *_pager, *_preempter, *_ext_preempter;
  Thread *present_next, *present_prev;
  Irq_alloc *_irq;

  // long ipc state
  L4_rcv_desc _target_desc;	// ipc buffer in receiver's address space
  unsigned _pagein_status_code;

  Address _vm_window0, _vm_window1; // data windows for the
  				// IPC partner's address space (for long IPC)
  jmp_buf *_recover_jmpbuf;	// setjmp buffer for page-fault recovery
  L4_timeout _pf_timeout;	// page-fault timeout specified by partner

  // debugging stuff
  Address _last_pf_address;
  unsigned _last_pf_error_code;

  unsigned _magic;
  static const unsigned magic = 0xf001c001;

  // Constructor
  Thread (Space* space, L4_uid id);

  // Thread killer
  void kill_all();
};

IMPLEMENTATION:

#include <cstdio>
#include <cstdlib>		// panic()
#include "atomic.h"
#include "entry_frame.h"
#include "fpu_alloc.h"
#include "globals.h"
#include "irq_alloc.h"
#include "kdb_ke.h"
#include "kmem.h"
#include "kmem_alloc.h"
#include "logdefs.h"
#include "map_util.h"
#include "sched_context.h"
#include "thread_state.h"
#include "thread_util.h"
#include "timer.h"

/** Class-specific allocator.
    This allocator ensures that threads are allocated at a fixed virtual
    address computed from their thread ID.
    @param id thread ID
    @return address of new thread control block
 */
PUBLIC inline 
void *
Thread::operator new(size_t, Threadid id)
{
  // Allocate TCB in TCB space.  Actually, do not allocate anything,
  // just return the address.  Allocation happens on the fly in
  // Thread::handle_page_fault().
  return static_cast<void*>(id.lookup());
}

/** Deallocator.  This function currently does nothing: We do not free up
    space allocated to thread-control blocks.
 */
PUBLIC inline
void 
Thread::operator delete(void *)
{
  // XXX should check if all thread blocks on a given page are free
  // and deallocate (or mark free) the page if so.  this should be
  // easy to detect since normally all threads of a given task are
  // destroyed at once at task deletion time
}

/** Cut-down version of Thread constructor; only for kernel threads
    Do only what's necessary to get a kernel thread started --
    skip all fancy stuff, no locking is necessary.
    @param space the address space
    @param id user-visible thread ID of the sender
 */
IMPLEMENT
Thread::Thread (Space* space, L4_uid id)
      : Receiver    (&_thread_lock, space,
                     Config::kernel_prio, Config::kernel_mcp,
                     Config::default_time_slice),
        Sender         (id),
        _preemption    (id),
        _sched_timeout (this),
        _space         (space),
        _magic         (magic)
{
  *reinterpret_cast<void(**)()>(--kernel_sp) = user_invoke;

  if (Config::stack_depth)
    std::memset((char*)this + sizeof(Thread), '5',
		Config::thread_block_size-sizeof(Thread)-64);
}

/** Destructor.  Reestablish the Context constructor's precondition.
    @pre current() == thread_lock()->lock_owner()
         && state() == Thread_dead
    @pre lock_cnt() == 0
    @post (kernel_sp == 0)  &&  (* (stack end) == 0)  &&  !exists()
 */
PUBLIC virtual
Thread::~Thread()		// To be called in locked state.
{
  assert(current() == thread_lock()->lock_owner());
  assert(state() == Thread_dead);
  assert(_magic == magic);

  unsigned long *init_sp = reinterpret_cast<unsigned long*>
    (reinterpret_cast<unsigned long>(this) + size - sizeof(Entry_frame));

  kernel_sp = 0;
  *--init_sp = 0;
  state_change (0, Thread_invalid);
  Fpu_alloc::free_state(fpu_state());

  // If the global timeslice on the CPU belongs to this dying thread,
  // we must invalidate it and force the selection of a new timeslice.
  if (current_sched() == sched())
    set_current_sched(0);

  thread_lock()->clear();

  // NOTE: It is possible that current() (this's locker) is deleted
  // right now (after it has cleared the lock), prior to invoking
  // Context's destructor.  Make sure all superclass destructors are
  // empty!  They might never be called!
}

/** Lookup function: Find Thread instance that owns a given Context. 
    @param c a context
    @return the thread that owns the context
 */
PUBLIC static inline
Thread*
Thread::lookup (Context* c)
{
  return reinterpret_cast<Thread*>(c);
}

/** Currently executing thread.
    @return currently executing thread.
 */
inline
Thread*
current_thread ()
{
  return Thread::lookup(current ());
}

//
// state requests/manipulation
//

/** Address space.
    @return pointer to thread's address space.
 */
PUBLIC inline 
Space *
Thread::space() const
{ 
  return _space; 
}

/** Thread lock.
    Overwrite Context's version of thread_lock() with a semantically
    equivalent, but more efficient version.
    @return lock used to synchronize accesses to the thread.
 */
PUBLIC inline 
Thread_lock *
Thread::thread_lock()
{ 
  return &_thread_lock; 
}

IMPLEMENT inline NEEDS ["timer.h"]
void
Thread::handle_timer_interrupt()
{  
  // Advance system clock
  Timer::update_system_clock();

  // Check if we need to reschedule due to timeouts or wakeups
  bool reschedule = Timeout::do_timeouts();

  Timer::acknowledge();

  // Timeslice handling for current thread
  Context::timer_tick (reschedule);
}

IMPLEMENT inline
void
Thread::preemption_event (Sched_context *sched)
{
  _preemption.send (_preempter, sched);
};
