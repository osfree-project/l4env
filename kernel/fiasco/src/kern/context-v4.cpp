INTERFACE:

EXTENSION class Context
{
protected:
  L4_uid _local_id;
};

IMPLEMENTATION[v4]:

#include "entry_frame.h"
#include "atomic.h"

/** Initialize a context.  After setup, a switch_to to this context results 
    in a return to user code using the return registers at regs().  The 
    return registers are not initialized however; neither is the space_context
    to be used in thread switching (use set_space_context() for that).
    @pre (kernel_sp == 0)  &&  (* (stack end) == 0)
    @param thread_lock pointer to lock used to lock this context 
    @param local_id pointer to the context's UTCB
    @param space_context the space context 
 */
PUBLIC inline NEEDS["entry_frame.h", "atomic.h"]
Context::Context (Thread_lock *thread_lock,
		  Space_context* space_context,
		  const L4_uid& local_id,
		  unsigned short prio,
                  unsigned short mcp,
		  unsigned short timeslice)
  : _space_context (space_context),
    _thread_lock (thread_lock),
    _timesharing_sched (this, 0, prio, timeslice),
    _sched (&_timesharing_sched),
    _mcp (mcp),
    _local_id (local_id)
{
  // NOTE: We do not have to synchronize the initialization of
  // _space_context because it is constant for all concurrent
  // invocations of this constructor.  When two threads concurrently
  // try to create a new task, they already synchronize in
  // sys_task_new() and avoid calling us twice with different
  // space_context arguments.

  Mword *init_sp = reinterpret_cast<Mword*>
    (reinterpret_cast<Mword>(this) + size - sizeof(Entry_frame));

  // don't care about errors: they just mean someone else has already
  // set up the tcb

  smp_cas(&kernel_sp, (Mword*)0, init_sp);
}

PROTECTED inline
L4_uid Context::local_id()
{ 
  return _local_id; 
}
