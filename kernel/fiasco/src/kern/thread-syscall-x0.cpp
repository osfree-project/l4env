IMPLEMENTATION[syscall-x0]:

#include "l4_types.h"
#include "entry_frame.h"
#include "thread.h"
#include "thread_util.h"
#include "task.h"
#include "space_index.h"
#include "space_index_util.h"

/** L4 system call task_new.  */
IMPLEMENT inline NOEXPORT
void Thread::sys_task_new()
{
  Sys_task_new_frame *regs = sys_frame_cast<Sys_task_new_frame>(this->regs());

  L4_uid id = regs->dest();
  unsigned taskno = id.task();
  Space_index si = Space_index(taskno);

  Space *s;

  for (;;)
    {
      if (Space_index_util::chief(si) != space_index())
	{
	  if (Config::conservative)
	    kdb_ke("task new: not chief");
	  break;
	}

      if ((s = si.lookup()))
	{
	  //
	  // task exists -- delete it
	  //

	  if (! lookup_first_thread(space_index())->kill_task(si))
	    {
	      printf("KERNEL: deletion of task 0x%x failed\n",
		     unsigned(si));
	      break;
	    }

	  // The subtask's address space has already been deleted by
	  // kill_task().
	}
      
      if (!regs->has_pager())
	{
	  //
	  // do not create a new task -- transfer ownership
	  //
	  L4_uid e = regs->new_chief();
	  
	  if (! si.set_chief(space_index(), Space_index(e.task())))
	    break;		// someone else was faster
	  
	  id.lthread(0);
	  id.chief(si.chief());

	  Space_index c = si.chief();
	  while (Space_index_util::chief(c) != Config::boot_id.chief())
	    c = Space_index_util::chief(c);

	  regs->new_taskid( id );

	  return;
	}
      
      //
      // create the task
      //
      s = new Task(taskno);
      if (! s) 
	break;
      
      if (si.lookup() != s)
	{
	  // someone else has been faster that in creating this task!
	  
	  delete s;
	  continue;		// try again
	}
      
      id.lthread(0);
      id.chief(space_index());
        
      //
      // create the first thread of the task
      //
      Thread *t = new (id) Thread (s, id, sched()->prio(),
				   mcp() < regs->mcp() ? mcp() : regs->mcp());

      check(t->initialize(regs->ip(), regs->sp(), 
			  Threadid(regs->pager()).lookup(), 0));
      
      return;
    }

  regs->new_taskid( L4_uid::NIL );

  //  return (Mword)-1;		// spec says return value is undefined
}

extern "C" void sys_task_new_wrapper()
{
  current_thread()->sys_task_new();
}
