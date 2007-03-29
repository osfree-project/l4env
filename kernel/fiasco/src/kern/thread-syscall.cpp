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
#include "warn.h"

KIP_KERNEL_FEATURE("pagerexregs");

/**
 * L4 system-call thread_switch.
 */
IMPLEMENT inline NOEXPORT
void
Thread::sys_thread_switch()
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
{ Proc::sti(); current_thread()->sys_thread_switch(); }

/*
 * Return current timesharing parameters
 */
PRIVATE inline
void
Thread::get_timesharing_param (L4_sched_param *param,
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
Thread::set_schedule_param (L4_sched_param param, unsigned short const id)
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
Thread::set_preempter (Thread* p)
{
  if (p && p->exists())
    _ext_preempter = p;
}

/*
 * Add a realtime timeslice at the end of the list
 */
PRIVATE inline
Mword
Thread::set_realtime_param (L4_sched_param param)
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
Thread::remove_realtime_param()
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
Thread::begin_periodic (Unsigned64 clock, Mword const type)
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
Thread::end_periodic()
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


PRIVATE inline
bool
Thread::ex_regs_permission_inter_task(Sys_ex_regs_frame *regs,
				      L4_uid *dst_id, Thread **dst,
				      Task **dst_task, Thread **dst_thread0)
{
  // inter-task ex-regs: also consider the task number
  if (EXPECT_TRUE(!regs->task()
	|| regs->task() == id().task()))
    return true;		// local ex_regs

  *dst_thread0 = lookup (L4_uid (regs->task(), 0), space());

  if (!*dst_thread0)
    return false;		// error

  {
    // avoid race between checking existence of thread and getting the
    // thread lock
    Lock_guard<Cpu_lock> guard(&cpu_lock);

    if (EXPECT_FALSE(!(*dst_thread0)->is_tcb_mapped()
	  || !(*dst_thread0)->exists()))
      return false;

    (*dst_thread0)->thread_lock()->lock_dirty();
  }

  // now we own the lock of thread 0 of the target task

  // take thread 0 of target as the ID template
  *dst_id = (*dst_thread0)->id();
  dst_id->lthread(regs->lthread());

  // Are we the pager of dst_id (just check address spaces)?
  Thread *dst_check = *dst = id_to_tcb(*dst_id);

  // if dst_check->_pager is nil (i.e. the destination thread does not
  // exist), we check the pager of thread0 of that address space
  if (!dst_check || !dst_check->is_tcb_mapped() 
      || !dst_check->_pager || !(*dst)->in_present_list())
    dst_check = *dst_thread0;

  if (dst_check->_pager->id().task() != id().task()
#ifdef CONFIG_TASK_CAPS
      && dst_check->_cap_handler->id().task() != id().task()
#endif
      )
    {
      WARN("Security violation: Not fault handler of %x.", dst_id->task());
      (*dst_thread0)->thread_lock()->clear();
      regs->old_eflags (~0U);
      return false;		// error
    }

  *dst_task = (*dst_thread0)->task();
  return true;			// success
}

/** L4 system call lthread_ex_regs.  */
IMPLEMENT inline NOEXPORT ALWAYS_INLINE
void
Thread::sys_thread_ex_regs()
{
  Sys_ex_regs_frame *regs = sys_frame_cast<Sys_ex_regs_frame>(this->regs());
  L4_uid dst_id  = id();
  dst_id.lthread (regs->lthread());
  Task *dst_task = _task;
  Thread *dst    = lookup (dst_id, space()), *dst_thread0 = 0;

  if (EXPECT_FALSE(! ex_regs_permission_inter_task(regs, &dst_id, &dst,
						   &dst_task, &dst_thread0)))
    {
      LOG_THREAD_EX_REGS_FAILED;
      regs->old_eflags(~0U);
      return;
    }

  dst = Thread::create(dst_task, dst_id, sched()->prio(), mcp());
  if (!dst)
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
Thread::sys_fpage_unmap()
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
Thread::sys_id_nearest()
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

      gateway = chief_index();
      ret = L4_NC_OUTER_CLAN;
    }
  while (false);

  regs->nearest( lookup_first_thread(gateway)->id() );
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
Thread::sys_thread_schedule()
{
  Sys_thread_schedule_frame *regs = sys_frame_cast
 <Sys_thread_schedule_frame>(this->regs());

  Thread *dst = lookup (regs->dst(), space());

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
Thread::share_quota(Task_num ref)
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
Thread::new_quota(Task_num ref, unsigned long max)
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
Thread::sys_task_new()
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

	  if (! si.set_chief(space_index(), Space_index(e.task())))
	    break;		// someone else was faster

	  id.lthread(0);
	  id.chief(si.chief());
	  reset_nest (id);

	  Space_index c = si.chief();
	  while (Space_index_util::chief(c) != Config::boot_id.chief())
	    {
	      c = Space_index_util::chief(c);
	      inc_nest (id);
	    }

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
      id.chief(space_index());
      update_nest (id);

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
{ Proc::sti(); current_thread()->sys_fpage_unmap(); }

extern "C" void sys_id_nearest_wrapper()
{ Proc::sti(); current_thread()->sys_id_nearest(); }

extern "C" void sys_thread_ex_regs_wrapper()
{ Proc::sti(); current_thread()->sys_thread_ex_regs(); }

extern "C" void sys_task_new_wrapper()
{ Proc::sti(); current_thread()->sys_task_new(); }

extern "C" void sys_thread_schedule_wrapper()
{ Proc::sti(); current_thread()->sys_thread_schedule(); }


//---------------------------------------------------------------------------
IMPLEMENTATION [v2]:

#include "l4_types.h"
#include "config.h"
#include "space_index.h"


PRIVATE inline NOEXPORT
void
Thread::reset_nest (L4_uid& id)
{ id.nest (0); }

PRIVATE inline NOEXPORT
void
Thread::inc_nest (L4_uid& id)
{ id.nest (id.nest() + 1); }

PRIVATE inline NOEXPORT 
void 
Thread::update_nest (L4_uid& id)
{
  id.nest ((nest() == 0 && space_index() == Config::boot_taskno)
	   ? 0 : nest() + 1);
}


//---------------------------------------------------------------------------
IMPLEMENTATION [x0]:

#include "l4_types.h"

PRIVATE inline NOEXPORT
void
Thread::reset_nest (L4_uid&)
{}

PRIVATE inline NOEXPORT
void
Thread::inc_nest (L4_uid&)
{}

PRIVATE inline NOEXPORT
void
Thread::update_nest (L4_uid&)
{}


//---------------------------------------------------------------------------
IMPLEMENTATION[arm]:

/**
 * Round quantum up to the nearest supported value.
 */
PRIVATE static inline NEEDS ["config.h"]
Unsigned64
Thread::round_quantum (Unsigned64 quantum)
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
Thread::round_quantum (Unsigned64 quantum)
{
  return quantum + Config::scheduler_granularity - 1
       - mod32 (quantum + Config::scheduler_granularity - 1,
	        Config::scheduler_granularity);
}
