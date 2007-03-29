IMPLEMENTATION[!caps]:

/**
 * Calculate TCB pointer from thread ID.
 *
 * @param id thread ID.
 * @param s space, needed if ID is local.
 * @return thread pointer if the ID was valid, 0 otherwise.
 */
PUBLIC inline			//also: NEEDS[Thread::lookup]; preprocess bug 
Thread *
Thread::lookup (L4_uid id, const Space*) const
{ return id_to_tcb (id); }

PUBLIC inline
bool
Thread::revalidate (const Thread* t, bool /*fail*/ = false) const
{
  return t;
}

PRIVATE inline
void
Thread::caps_init(const Thread* /*creator*/) const
{}

PUBLIC inline
void
Thread::setup_task_caps (const Task* /*new_task*/, 
			 const Sys_task_new_frame * /*params*/,
			 const Space */*cap_handler*/) const
{
}

//////////////////////////////////////////////////////////////////////

IMPLEMENTATION[caps]:

#include "map_util.h"
#include "paging.h"
#include "task.h"

EXTENSION class Thread
{
  bool _cap_no_fault;
};

/**
 * Calculate TCB pointer from thread ID.
 *
 * @param id thread ID.
 * @param s space, needed if ID is local.
 * @return thread pointer if the ID was valid, 0 otherwise.
 */
PUBLIC inline NEEDS["task.h"]	//also: NEEDS[Thread::lookup]; preprocess bug 
Thread *
Thread::lookup (L4_uid id, Space* s)
{ 
  assert (s == task());

  if (EXPECT_FALSE (id.is_invalid()))
    return 0;

  if (EXPECT_FALSE (s->task_caps_enabled()))
    if (EXPECT_FALSE (! s->cap_space()->cap_lookup (id.task())))
      if (! cap_fault (id.task()))
	return 0;

  return id_to_tcb (id); 
}

PUBLIC inline
bool
Thread::revalidate (Thread* t, bool /*fail*/ = false)
{
  if (! t || ! t->exists())
    return false;

  Space* s = task();

  if (EXPECT_FALSE (s->task_caps_enabled()))
    {
      Task_num task = t->id().task();

      if (EXPECT_FALSE (! s->cap_space()->cap_lookup (task)))
	{
	  if (_cap_no_fault)
	    return false;
	  
	  return cap_fault (task);
	}
    }
  return true;
}

PRIVATE
bool
Thread::cap_fault (Task_num taskno)
{
  Space *s = task();

  Mword error_code = PF::write_error();
  if (! (state() & Thread_ipc_mask))
    error_code |= PF::usermode_error();


  _cap_no_fault = true;	// Prevent faults if _cap_handler doesn't validate

  bool was_locked = cpu_lock.test();
  if (was_locked) cpu_lock.clear();

  Ipc_err err 
    = handle_page_fault_pager (_cap_handler, 
			       L4_fpage::task_cap (taskno, 1, 0).raw(),
			       error_code);

  if (was_locked) cpu_lock.lock();

  _cap_no_fault = false;

  if (err.has_error())
    return false;

  return s->cap_space()->cap_lookup (taskno);
}

PRIVATE inline
void
Thread::caps_init(Thread* creator)
{
  _cap_no_fault = false;

  if (creator->task()->task_caps_enabled())
    _cap_handler = creator->_cap_handler;
  else
    _cap_handler = 0;
}

PUBLIC inline NEEDS["map_util.h"]
void
Thread::setup_task_caps (Task* new_task, const Sys_task_new_frame *params, 
                         const Space *cap_handler)
{
  Space* s = space();

  if ((params && params->cap_handler(utcb()).is_valid()) 
      || s->task_caps_enabled())
    {
      {
	// XXX This should be done in sys_task_new and cover all newly
	// allocated resources.
	Lock_guard<Thread_lock> guard(::current()->thread_lock());
	
	new_task->enable_task_caps();
      }

      // Map task capabilities for new task itself, parent task, and task 0.

      map (cap_mapdb_instance(), 
	   s->cap_space(), s->id(), s->id(), 1,
	   new_task->cap_space(), new_task->id(), s->id(), 
	   false, 0, 0);

      map (cap_mapdb_instance(), 
	   s->cap_space(), s->id(), new_task->id(), 1,
	   new_task->cap_space(), new_task->id(), new_task->id(), 
	   false, 0, 0);

      map (cap_mapdb_instance(), 
	   s->cap_space(), s->id(), 0, 1,
	   new_task->cap_space(), new_task->id(), 0, 
	   false, 0, 0);

      if (cap_handler 
	  && cap_handler->id() != s->id()) // optimztn.: Do not insert s twice
        map (cap_mapdb_instance(),
	     s->cap_space(), s->id(), cap_handler->id(), 1,
	     new_task->cap_space(), new_task->id(), cap_handler->id(),
	     false, 0, 0);
    }
  else
    {
      // Map everything.
      map (cap_mapdb_instance(), 
	   s->cap_space(), s->id(), 0, L4_uid::Max_tasks,
	   new_task->cap_space(), new_task->id(), 0, 
	   false, 0, 0);
    }    
}
