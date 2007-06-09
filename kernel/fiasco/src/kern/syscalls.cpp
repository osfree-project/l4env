INTERFACE:

class Sys_ex_regs_frame;


IMPLEMENTATION:

#include <cstdio>

#include "l4_types.h"

#include "config.h"
#include "entry_frame.h"
#include "feature.h"
#include "irq.h"
#include "irq_alloc.h"
#include "logdefs.h"
#include "map_util.h"
#include "ram_quota.h"
#include "ram_quota_alloc.h"
#include "space.h"
#include "space_index.h"
#include "space_index_util.h"
#include "std_macros.h"
#include "task.h"
#include "thread.h"
#include "thread_state.h"
#include "warn.h"

class Syscalls : public Thread
{

public:
  void sys_fpage_unmap();
  void sys_thread_switch();
  void sys_thread_schedule();
  void sys_task_new();
  void sys_id_nearest();
  void sys_thread_ex_regs();
protected:
  Syscalls() : Thread(0,0) {}
};

KIP_KERNEL_FEATURE("pagerexregs");

/**
 * L4 system-call thread_switch.
 */
IMPLEMENT inline NOEXPORT
void
Syscalls::sys_thread_switch()
{
  Sys_thread_switch_frame *regs = sys_frame_cast
 <Sys_thread_switch_frame>(this->regs());

  // Protect against timer interrupt
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  if (regs->dst().is_nil())
    {
      // Return error if user and kernel disagree on the timeslice ID
      if (regs->id() != sched()->id())
        {
          regs->ret (~0U);      // Failure
          return;
        }

      // Compute remaining quantum length of timeslice
      regs->left (sched() == current_sched()
                           ? timeslice_timeout->get_timeout()
                           : sched()->left());

      // Yield own current timeslice
      switch_sched (sched()->id() ? sched()->next() : sched());
    }
  else
    {
      Thread *t = lookup (regs->dst(), space());

      if (t && t != this && (t->state() & Thread_ready))
        {
          switch_exec_locked (t, Not_Helping);
          regs->left (0);		// Assume timeslice was used up
          regs->ret  (0);		// Success
          return;
        }

      // Compute remaining quantum length of timeslice
      regs->left (timeslice_timeout->get_timeout());

      // Yield current global timeslice
      current_sched()->owner()->switch_sched (current_sched()->id()   ?
                                              current_sched()->next() :
                                              current_sched());
    }

  schedule();

  regs->ret (0);			// Success
}

extern "C" void sys_thread_switch_wrapper()
{ Proc::sti(); static_cast<Syscalls*>(current_thread())->sys_thread_switch(); }

/*
 * Return current timesharing parameters
 */
PRIVATE inline
void
Syscalls::get_timesharing_param (L4_sched_param *param,
                               L4_uid *preempter,
                               L4_uid *ipc_partner)
{
  Mword s = state();

  *preempter = _ext_preempter ? _ext_preempter->id() : L4_uid::Invalid;

  *ipc_partner = s & (Thread_polling | Thread_receiving) && partner() ?
                      partner()->id() : L4_uid::Invalid;

  if (s & Thread_dead)
    s = 0xf;
  else if (s & Thread_polling)
    s = (s & Thread_ready) ? 8 : 0xd;
  else if (s & Thread_receiving)
    s = (s & Thread_ready) ? 8 : 0xc;
  else
    s = 0;

  param->prio (sched_context()->prio());
  param->time (sched_context()->quantum());
  param->thread_state (s);
}

/*
 * Set scheduling parameters for timeslice with id 'id'
 */
PRIVATE inline
Mword
Syscalls::set_schedule_param (L4_sched_param param, unsigned short const id)
{
  if (EXPECT_FALSE (param.prio() > thread_lock()->lock_owner()->mcp()))
    return ~0U;

  // We need to protect the priority manipulation so that this thread
  // cannot be preempted and ready-enqueued according to a wrong
  // priority and the current timeslice cannot change while we are
  // manipulating it

  Lock_guard <Cpu_lock> guard (&cpu_lock);

  Sched_context *s = sched_context (id);
  if (!s)
    return ~0U;

  if (s->prio() != param.prio())
    {
      // Dequeue from ready-list if we manipulate the current time slice
      if (s == sched())
        ready_dequeue();

      // ready_enqueue happens during thread_lock release
      s->set_prio (param.prio());
    }

  Unsigned64 q = param.time();
  if (q != (Unsigned64) -1)
    {
      q = round_quantum (q);
      s->set_quantum (q);
      if (s != sched())
        s->set_left (q);
    }

  return 0;
}

/*
 * Set preempter unless "invalid"
 */
PRIVATE inline
void
Syscalls::set_preempter (Thread* p)
{
  if (p && p->exists())
    _ext_preempter = p;
}

/*
 * Add a realtime timeslice at the end of the list
 */
PRIVATE inline
Mword
Syscalls::set_realtime_param (L4_sched_param param)
{
  if (EXPECT_FALSE (mode() & Periodic || _deadline_timeout.is_set() ||
      param.prio() > thread_lock()->lock_owner()->mcp() ||
      param.time() == (Unsigned64) -1))
    return ~0U;

  Sched_context *s = new Sched_context (this,
                                        sched_context()->prev()->id() + 1,
                                        param.prio(),
                                        round_quantum (param.time()));

  s->enqueue_before (sched_context());

  return 0;
}

/*
 * Remove all realtime timeslices
 */
PRIVATE inline
Mword
Syscalls::remove_realtime_param()
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  if (EXPECT_FALSE (mode() & Periodic || _deadline_timeout.is_set()))
    return ~0U;

  assert (sched() == sched_context());

  preemption()->set_pending (0);
  preemption()->sender_dequeue (preemption()->receiver()->sender_list());

  Sched_context *s, *tmp;

  for (s  = sched_context()->next();
       s != sched_context();
       tmp = s, s = s->next(), tmp->dequeue(), delete tmp);

  return 0;
}

PRIVATE
Mword
Syscalls::begin_periodic (Unsigned64 clock, Mword const type)
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  // Refuse to enter periodic mode when in or transitioning to periodic mode
  if (EXPECT_FALSE ((mode() & Periodic) || _deadline_timeout.is_set()))
    return ~0U;

  // clock == 0 means start period right now
  if (EXPECT_FALSE (!clock))
    clock = Kip::k()->clock;

  // Refuse to enter periodic mode if clock is in the past
  if (EXPECT_FALSE (clock < Kip::k()->clock))
    return ~0U;

  assert (sched() == sched_context());

  // Switch to real-time mode
  set_mode (Sched_mode (type));

  // Program deadline timeout
  _deadline_timeout.set (clock);

  return 0;
}

PRIVATE
Mword
Syscalls::end_periodic()
{
  Lock_guard <Cpu_lock> guard (&cpu_lock);

  // Refuse to leave periodic mode unless in or transitioning to periodic mode
  if (EXPECT_FALSE (!(mode() & Periodic) && !_deadline_timeout.is_set()))
    return ~0U;

  // Switch to time-sharing mode
  set_mode (Sched_mode (0));

  // Switch to time-sharing Sched_context
  if (sched() != sched_context())
    switch_sched (sched_context());

  // Cancel pending deadline timeout
  _deadline_timeout.reset();

  // Undelay if the target thread is in a next-period IPC
  switch (state() & (Thread_delayed_deadline | Thread_delayed_ipc))
    {
      case Thread_delayed_deadline | Thread_delayed_ipc:
        state_del (Thread_delayed_deadline);
        break;

      case Thread_delayed_deadline:
        state_change_dirty (~Thread_delayed_deadline, Thread_ready);
        ready_enqueue();
        break;
    }

  // Priority check and potential thread switch happens on thread_lock release

  return 0;
}



/** L4 system call lthread_ex_regs.  */
IMPLEMENT inline NOEXPORT ALWAYS_INLINE
void
Syscalls::sys_thread_ex_regs()
{
  Sys_ex_regs_frame *regs = sys_frame_cast<Sys_ex_regs_frame>(this->regs());
  L4_uid dst_id  = id();
  dst_id.lthread (regs->lthread());
  Task *dst_task = _task;
  Thread *dst    = lookup (dst_id, space()), *dst_thread0 = 0;

  if (EXPECT_FALSE(! ex_regs_permission_inter_task(regs, &dst_id, &dst,
						   &dst_task, &dst_thread0))
      || !(dst = Thread::create(dst_task, dst_id, sched()->prio(), mcp())))
    {
      LOG_THREAD_EX_REGS_FAILED;
      regs->old_eflags(~0U);
      return;
    }

  // now we own the lock of an existing thread in the destination task,
  // hence we can release the lock of dst_thread0
  if (dst_thread0 && dst_thread0 != dst)
    dst_thread0->thread_lock()->clear();

  // o_pager and o_preempter don't need to be initialized since they are
  // only evaluated if new_thread is false
  Thread *o_pager, *o_cap_handler;
  Receiver *o_preempter;
  Mword o_flags;

  LOG_THREAD_EX_REGS;

  // initialize the thread with new values.

  // undocumented but jochen-compatible: if we're creating a new
  // thread (and aren't modifying an existing one), don't change the
  // register parameters that have been passed to us to ``old'' values
  // (i.e., all zeroes)
  Mword o_ip, o_sp;
  if (dst->initialize(regs->ip(), regs->sp(),
		      lookup (regs->pager(), space()),
		      lookup (regs->preempter(), space()),
		      lookup (regs->cap_handler(utcb()), space()),
		      &o_ip, &o_sp,
		      &o_pager,
		      &o_preempter,
		      &o_cap_handler,
		      &o_flags,
		      regs->no_cancel(),
		      regs->alien(),
		      regs->trigger_exception()))
    {
      regs->old_sp        (o_sp);
      regs->old_ip        (o_ip >= Mem_layout::User_max ? ~0UL : o_ip);
      regs->old_pager     (o_pager ? o_pager->id() : L4_uid::Invalid);
      regs->old_preempter (o_preempter
	  ? Thread::lookup (context_of (o_preempter))->id()
	  : L4_uid::Invalid);
      regs->old_cap_handler (o_cap_handler 
	  ? o_cap_handler->id() : L4_uid::Invalid,
	  utcb());
      regs->old_eflags    (o_flags);

    }
  else
    regs->old_eflags (~0U);	// error

  dst->thread_lock()->clear();
}

/** L4 system call fpage_unmap.  */
IMPLEMENT inline
void
Syscalls::sys_fpage_unmap()
{
  Sys_unmap_frame *regs = sys_frame_cast<Sys_unmap_frame>(this->regs());
 
  unsigned what = 0;

  if (! regs->no_unmap())
    {
      if (regs->downgrade())
	what |= Unmap_w;
      else
	what |= Unmap_w | Unmap_r;
    }

  if (regs->reset_references())
    what |= Unmap_referenced | Unmap_dirty;

  unsigned status = 0;

  if (what)
    {
      unsigned flushed_rights = 
	fpage_unmap (space(), regs->fpage(), regs->self_unmap(), 
		     regs->self_unmap() ? 0 : regs->restricted(), what);

      if (flushed_rights & Unmap_referenced)
	status |= L4_fpage::Referenced;
      
      if (flushed_rights & Unmap_dirty)    
	status |= L4_fpage::Dirty;
    }

  regs->ret (status);
}

/** L4 system call id_nearest.  */
IMPLEMENT inline NOEXPORT ALWAYS_INLINE
void
Syscalls::sys_id_nearest()
{
  enum {
    L4_NC_SAME_CLAN  = 0x00,	/* destination resides within the */
                                /* same clan */
    L4_NC_INNER_CLAN = 0x0C,	/* destination is in an inner clan */
    L4_NC_OUTER_CLAN = 0x04,	/* destination is outside the */
				/* invoker's clan */
  };

  Sys_id_nearest_frame *regs = sys_frame_cast<Sys_id_nearest_frame>(this->regs());
  L4_uid dst_id_long = regs->dst();

  LOG_ID_NEAREST;

  if (dst_id_long.is_nil())
    {
      regs->nearest (id());
      regs->type (L4_NC_SAME_CLAN);
      return;
    }

  // special case for undocumented Jochen-compatibility: if id ==
  // invalid_id, deliver our chief
  if (dst_id_long.is_invalid())
    {
      regs->nearest (lookup_first_thread (chief_index())->id());
      regs->type (L4_NC_OUTER_CLAN);
      return;
    }

  Mword ret;

  Space_index dst_task = Space_index(dst_id_long.task());
  Space_index gateway = dst_task;

  // don't trust the id value the user sent us, except for the task
  // and thread id

  do
    {
      gateway = dst_task;

      if (dst_task == chief_index()
	  || Space_index_util::chief(dst_task) == chief_index()
	  || Space_index_util::chief(dst_task) == space_index())
	{
	  ret = L4_NC_SAME_CLAN;
	  break;
	}

      while (Space_index_util::chief(gateway) != gateway)
	{
	  gateway = Space_index_util::chief(gateway);
	  if (Space_index_util::chief(gateway) == space_index())
	    {
	      ret = L4_NC_INNER_CLAN;
	      break;
	    }
	}

      if (Space_index_util::chief(gateway) == space_index())
	break;
      
      gateway = chief_index();
      ret = L4_NC_OUTER_CLAN;
    }
  while (false);

  regs->nearest(L4_uid((unsigned)gateway, 0, 0));
  regs->type (ret);
  return;

#if 0
  Thread *partner = me->find_partner(dst_id);
  if (partner)
    {
      *dst_id_long = partner->id();
      return partner->chief() == me->chief() ?
        0 : (L4_IPC_REDIRECT_MASK
             | ((partner->chief() == me->space()) ?
                L4_IPC_SRC_MASK : 0));
    }

  /* non-existent thread which would belong to our clan? */
  if (me->chief() == dst_id_long->id.chief())
    return 0;

  /* target hasn't yet been assiged to a clan -- so it belongs to the
     outermost clan */
  if (me->nest() > 0)
    {
      // send to our chief
      *dst_id_long = lookup_first_thread(me->chief())->id();
      return L4_IPC_REDIRECT_MASK;
    }

  /* oh, we're in the outermost clan already, and we can't let the
     kernel redirect a message to this destination */
  return L4_IPC_Enot_existent;
#endif
}

/** L4 system call thread_schedule.  */
IMPLEMENT inline NOEXPORT ALWAYS_INLINE
void
Syscalls::sys_thread_schedule()
{
  Sys_thread_schedule_frame *regs = sys_frame_cast<Sys_thread_schedule_frame>(this->regs());

  Syscalls *dst = (Syscalls*)lookup (regs->dst(), space());

  if (EXPECT_FALSE (!dst))
    {
      regs->old_param (~0U);        // Return error
      return;
    }

  Lock_guard <Thread_lock> guard (dst->thread_lock());

  // Check that thread exists and prio doesn't exceed my MCP
  if (EXPECT_FALSE (!dst->exists() || dst->sched_context()->prio() > mcp()))
    {
      regs->old_param (~0U);        // Return error
      return;
    }

  L4_sched_param param;
  L4_uid preempter, partner;

  // Get return parameters
  dst->get_timesharing_param (&param, &preempter, &partner);

  // Set timeslice and priority
  if (regs->param().is_valid())
    {
      switch (regs->param().mode())
        {
          case 0:			// set timesharing timeslice
            if (EXPECT_FALSE (dst->set_schedule_param (regs->param(), 0) == ~0U))
              {
                regs->old_param (~0U);
                return;
              }
            dst->set_small_space (regs->param().small());
            break;

          case 1:			// set realtime timeslice
            regs->old_param (dst->set_realtime_param (regs->param()));
            return;

          case 2:			// remove realtime timeslices
            regs->old_param (dst->remove_realtime_param());
            return;

          case 3:			// set period length
            dst->set_period (round_quantum (regs->time()));
            regs->old_param (0);
            return;

          case 4:			// begin strictly periodic mode
            regs->old_param (dst->begin_periodic (regs->time(), 0));
            return;

          case 5:			// begin non-strictly periodic mode
            regs->old_param (dst->begin_periodic (regs->time(), Nonstrict));
            return;

          case 6:			// end periodic mode
            regs->old_param (dst->end_periodic());
            return;

         case 7:                       // change realtime timeslice
           regs->old_param (dst->set_schedule_param (regs->param(),
						     regs->param().small()));
           return;

    	 default:
	   WARN ("thread_schedule: param_word bits 16-19 must be 0. "
		 "Fix your bindings!");
	   if (Warning < Config::warn_level)
	     kdb_ke("stop");
	   return;
        }
    }

  // Respect the fact that the consumed time is only updated on context switch
  if (dst == current_thread())
    update_consumed_time();

  // Set preempter
  dst->set_preempter (lookup (regs->preempter(), space()));

  // Setup return parameters
  regs->old_param     (param);
  regs->old_preempter (preempter);
  regs->partner       (partner);
  regs->time          (dst->consumed_time());
}

//
// -- Quota helpers
//

PRIVATE
Ram_quota *
Syscalls::share_quota(Task_num ref)
{
  Space *ref_space = Space_index(ref).lookup();
  if (!ref_space 
      || (ref_space->chief() != space()->id()
	  && ref_space != space()))
    return 0;

  return ref_space->ram_quota();
}

PRIVATE
Ram_quota *
Syscalls::new_quota(Task_num ref, unsigned long max)
{
  Space *ref_space = Space_index(ref).lookup();
  if (!ref_space 
      || (ref_space->chief() != space()->id()
	  && ref_space != space()))
    return 0;

  return Ram_quota_alloc::alloc(ref_space->ram_quota(), max);
}


/** L4 system call task_new.  */
IMPLEMENT inline NOEXPORT ALWAYS_INLINE
void
Syscalls::sys_task_new()
{
  Sys_task_new_frame *regs = sys_frame_cast<Sys_task_new_frame>(this->regs());
  L4_uid id = regs->dst();
  unsigned taskno = id.task();
  Space_index si = Space_index(taskno);
  Task *task;

  LOG_TASK_NEW;

  for (;;)
    {
      if (Space_index_util::chief(si) != space_index())
	{
	  if (Config::conservative)
	    kdb_ke("task new: not chief");
	  break;
	}

      if (si.lookup())
	{
	  //
	  // task exists -- delete it
	  //
	  if (! lookup_first_thread(space_index())->kill_task(si))
	    {
	      WARN("Deletion of task 0x%x failed.", unsigned(si));
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
	  Space_index chief(e.task());

	  if (!chief.lookup())
	    break;

	  if (! si.set_chief(space_index(), chief))
	    break;		// someone else was faster

	  id.lthread(0);
	  regs->new_taskid(id);
	  return;
	}

      //
      // create the task
      //
      
      Ram_quota *rq = space()->ram_quota();
      L4_quota_desc q_desc = regs->quota_descriptor(utcb());
      //printf("task_new quota_desc = %08lx\n", q_desc);
      switch (q_desc.command())
	{
	case L4_quota_desc::Share:
	  rq = share_quota(q_desc.id());
	  break;
	case L4_quota_desc::New:
	  rq = new_quota(q_desc.id(), q_desc.amount());
	  break;
	default:
	  break;
	}

      if (! rq)
	break;

      task = Task::create(rq, taskno);
      if (! task)
	break;

      if (si.lookup() != task)
	{
	  // someone else has been faster that in creating this task!

	  delete task;
	  continue;		// try again
	}

      Thread* cap_handler = lookup (regs->cap_handler(utcb()), space());

      CNT_TASK_CREATE;
      if (! task->initialize())
	{
	  // out of mem
	  delete task;
	  regs->new_taskid(L4_uid::Nil);
	  return;
	}

      setup_task_caps (task, regs, cap_handler ? cap_handler->space() : 0);

      id.lthread(0);

      //
      // create the first thread of the task
      //

      // returns locked tcb
      Thread *t = Thread::create(task, id, sched()->prio(), 
	  mcp() < regs->mcp() ? mcp() : regs->mcp());

      if (EXPECT_FALSE(!t))
	{
	  delete task;
	  regs->new_taskid(L4_uid::Nil);
	  return;
	}

      check(t->initialize(regs->ip(), regs->sp(), 
			  lookup (regs->pager(), space()),
			  0, cap_handler,
			  0, 0, 0, 0, 0, 0, 0,
			  regs->alien(), regs->trigger_exception()));

      t->thread_lock()->clear();
      return;
    }

  regs->new_taskid (L4_uid::Nil);

  // return (Mword)-1;		// spec says return value is undefined!
}

// these wrappers must come last in the source so that the real sys-call
// implementations can be inlined by g++

extern "C" void sys_fpage_unmap_wrapper()
{ Proc::sti(); static_cast<Syscalls*>(current_thread())->sys_fpage_unmap(); }

extern "C" void sys_id_nearest_wrapper()
{ Proc::sti(); static_cast<Syscalls*>(current_thread())->sys_id_nearest(); }

extern "C" void sys_thread_ex_regs_wrapper()
{ Proc::sti(); static_cast<Syscalls*>(current_thread())->sys_thread_ex_regs(); }

extern "C" void sys_task_new_wrapper()
{ Proc::sti(); static_cast<Syscalls*>(current_thread())->sys_task_new(); }

extern "C" void sys_thread_schedule_wrapper()
{ Proc::sti(); static_cast<Syscalls*>(current_thread())->sys_thread_schedule(); }


//-------------------------------------------------------------------------
IMPLEMENTATION [ulock]:

#include "u_lock.h"
#include "u_semaphore.h"
#include "obj_ref_ptr.h"

KIP_KERNEL_FEATURE("ulock");
KIP_KERNEL_FEATURE("usemaphore");

PRIVATE inline NOEXPORT
void 
Syscalls::sys_u_lock(Sys_u_lock_frame *regs)
{
  Ref_ptr<U_lock> l;

  switch (regs->op())
    {
    case Sys_u_lock_frame::New:
	{
	  U_lock *l = U_lock::alloc(space()->ram_quota());
	  if (!l 
	      || !map(l, space()->obj_space(), space()->id(), regs->lock(), 0)) 
	    {
	      regs->result(4); // nomem
	      return;
	    } 
	  regs->result(0);
	  return;
	}

    case Sys_u_lock_frame::Lock:
    case Sys_u_lock_frame::Unlock:
	{
	  Lock_guard<Cpu_lock> guard(&cpu_lock);
	  l = space()->obj_space()->lookup<U_lock>(regs->lock());
	  if (!l)
	    {
	      regs->result(2);
	      return;
	    }
	}
      break;
    default:
      regs->result(3); 
      return;
    }

  //printf ("  do it (%p)\n", l);
  
  switch (regs->op())
    {
    case Sys_u_lock_frame::Lock:
      if (l->lock() == Obj_helping_lock::Invalid)
	{
	  regs->result(2);
	  return;
	}
      break;
    case Sys_u_lock_frame::Unlock:
      l->clear();
      break;
    default:
      break;
    }

  regs->result(0);
}

PRIVATE inline NOEXPORT
void 
Syscalls::sys_u_semaphore(Sys_u_lock_frame *regs)
{
  U_semaphore *l;
  Lock_guard<Cpu_lock> guard;

  switch (regs->op())
    {
    case Sys_u_lock_frame::New_semaphore:
	{
	  U_semaphore *l = U_semaphore::alloc(space()->ram_quota());
	  if (!l 
	      || !map(l, space()->obj_space(), space()->id(), regs->lock(), 0)) 
	    {
	      regs->result(4); // nomem
	      return;
	    } 
	  regs->result(0);
	  return;
	}

    case Sys_u_lock_frame::Sem_sleep:
    case Sys_u_lock_frame::Sem_wakeup:
      guard.lock(&cpu_lock);
      l = space()->obj_space()->lookup<U_semaphore>(regs->lock());
      if (!l)
	{
	  regs->result(2);
	  return;
	}
      break;
    default:
      regs->result(3); 
      return;
    }

  //printf ("  do it (%p)\n", l);
  unsigned long res;
  
  switch (regs->op())
    {
    case Sys_u_lock_frame::Sem_sleep:
      //LOG_MSG_3VAL(this, "USBLOCK", regs->timeout().raw(), 0, 0);
      regs->result(res = l->block_locked(regs->timeout(), regs->semaphore()));
      //LOG_MSG_3VAL(this, "USBLOCK+", res, 0, 0);
      return;
    case Sys_u_lock_frame::Sem_wakeup:
      //LOG_MSG(this, "USWAKE");
      l->wakeup_locked(regs->semaphore());
      break;
    default:
      break;
    }

  regs->result(0);
}

PUBLIC inline NOEXPORT
void
Syscalls::sys_u_lock()
{
  Sys_u_lock_frame *regs = sys_frame_cast<Sys_u_lock_frame>(this->regs());
  //printf("u_lock called(op=%d, lock=%ld)\n", regs->op(), regs->lock());
  //LOG_MSG_3VAL(this, "UL", regs->op(), regs->lock(), regs->timeout().raw());
  if (regs->op() < Sys_u_lock_frame::New_semaphore)
    sys_u_lock(regs);
  else
    sys_u_semaphore(regs);
}


extern "C" void sys_u_lock_wrapper()
{ Proc::sti(); static_cast<Syscalls*>(current_thread())->sys_u_lock(); }


//---------------------------------------------------------------------------
IMPLEMENTATION [!ulock]:

extern "C" void sys_u_lock_wrapper()
{ Proc::sti(); kdb_ke("USER BUG: not user-level locks implemented"); }


//---------------------------------------------------------------------------
IMPLEMENTATION [log]:

#include <alloca.h>
#include <cstring>
#include "config.h"
#include "jdb_trace.h"
#include "jdb_tbuf.h"
#include "types.h"
#include "cpu_lock.h"

/** L4 system call fpage_unmap.
 */
PUBLIC inline NOEXPORT ALWAYS_INLINE
void
Syscalls::sys_fpage_unmap_log()
{
  Entry_frame *ef       = reinterpret_cast<Entry_frame*>(this->regs());
  Sys_unmap_frame *regs = reinterpret_cast<Sys_unmap_frame*>(this->regs());

  if (Jdb_unmap_trace::log()
      && Jdb_unmap_trace::check_restriction(current_thread()->id(),
                                            regs->fpage().raw() & Config::PAGE_MASK))
    {
      Lock_guard <Cpu_lock> guard (&cpu_lock);

      Tb_entry_unmap *tb = static_cast<Tb_entry_unmap*>
	(EXPECT_TRUE(Jdb_unmap_trace::log_buf()) ? Jdb_tbuf::new_entry()
					     : alloca(sizeof(Tb_entry_unmap)));
      tb->set(this, ef->ip(), regs->fpage().raw(), regs->map_mask(), false);

      if (EXPECT_TRUE(Jdb_unmap_trace::log_buf()))
	Jdb_tbuf::commit_entry();
      else
	Jdb_tbuf::direct_log_entry(tb, "UNMAP");
    }

  sys_fpage_unmap_wrapper();
}


extern "C" void sys_fpage_unmap_log_wrapper(void)
{
  Proc::sti();
  static_cast<Syscalls*>(current_thread())->sys_fpage_unmap_log();
}


//---------------------------------------------------------------------------
IMPLEMENTATION[arm]:

/**
 * Round quantum up to the nearest supported value.
 */
PRIVATE static inline NEEDS ["config.h"]
Unsigned64
Syscalls::round_quantum (Unsigned64 quantum)
{
  return quantum ? (--quantum / Config::scheduler_granularity + 1)
                              * Config::scheduler_granularity : 0;
}


//---------------------------------------------------------------------------
IMPLEMENTATION[ia32,ux,amd64]:

#include "mod32.h"

/**
 * Round quantum up to the nearest supported value.
 */
PRIVATE static inline NEEDS ["config.h","mod32.h"]
Unsigned64
Syscalls::round_quantum (Unsigned64 quantum)
{
  return quantum + Config::scheduler_granularity - 1
       - mod32 (quantum + Config::scheduler_granularity - 1,
	        Config::scheduler_granularity);
}


//---------------------------------------------------------------------------
INTERFACE [ia32 || ux || amd64]:

extern void (*syscall_table[])();


//---------------------------------------------------------------------------
IMPLEMENTATION [ia32 || ux || amd64]:

void (*syscall_table[])() = 
{ 
  sys_ipc_wrapper,
  sys_id_nearest_wrapper,
  sys_fpage_unmap_wrapper,
  sys_thread_switch_wrapper,
  sys_thread_schedule_wrapper,
  sys_thread_ex_regs_wrapper,
  sys_task_new_wrapper,
#ifdef CONFIG_PL0_HACK
  sys_priv_control_wrapper,
#else
  0,
#endif
  0,
  sys_u_lock_wrapper
};

