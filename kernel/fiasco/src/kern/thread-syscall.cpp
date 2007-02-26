
INTERFACE:

class Entry_frame;

extern void (*syscall_table[])();

IMPLEMENTATION [syscall]:

#include <cstdio>

#include "l4_types.h"

#include "config.h"
#include "irq.h"
#include "irq_alloc.h"
#include "logdefs.h"
#include "map_util.h"
#include "space.h"
#include "std_macros.h"
#include "task.h"
#include "thread_util.h"

/** L4 system call fpage_unmap.  */
IMPLEMENT inline NOEXPORT
void
Thread::sys_fpage_unmap()
{
  Sys_unmap_frame *regs = sys_frame_cast<Sys_unmap_frame>(this->regs());
  fpage_unmap(space(), regs->fpage(),
	      regs->self_unmap(), regs->downgrade());
}

extern "C" void sys_fpage_unmap_wrapper()
{
  current_thread()->sys_fpage_unmap();
}

/** L4 system call thread_switch.  */
IMPLEMENT inline NOEXPORT
void
Thread::sys_thread_switch()
{
  Sys_thread_switch_frame *regs = sys_frame_cast<Sys_thread_switch_frame>(this->regs());

  // If someone calls thread_switch(L4_NIL_ID) we have to schedule
  // within the context of the calling thread. Otherwise we may end up
  // in an endless loop.  
  // (idle thread -> thread calling thread_switch -> other ready threads)
  // Therefore we have to check for L4_NIL_ID and can't rely on the
  // idle thread to do a reschedule for us.

  Thread *t = Threadid(regs->dest()).lookup();
  if (t && regs->has_dest() && (t->state() & Thread_running))
    switch_to(t);
  else
    {
      timeslice_ticks_left = 0;
      schedule();
    }
}

extern "C" void sys_thread_switch_wrapper()
{
  current_thread()->sys_thread_switch();
}

/*
 * Round up timeslice to the nearest possible value
 */
PRIVATE inline NOEXPORT
Unsigned64
Thread::round_timeslice (Unsigned64 timeslice)
{
  return (timeslice + Config::microsec_per_tick - 1) /
                      Config::microsec_per_tick;
}

/*
 * Return current timesharing parameters
 */
PRIVATE
void
Thread::get_timesharing_param (L4_sched_param *param,
                               L4_uid *preempter,
                               L4_uid *ipc_partner)
{
  Mword s = state();

  *preempter = _ext_preempter ? _ext_preempter->id() : L4_uid::INVALID;

  *ipc_partner = s & (Thread_polling | Thread_receiving) && partner() ?
                 partner()->id() : L4_uid::INVALID;

  if (s & Thread_dead)
    s = 0xf;
  else if (s & Thread_polling)
    s = (s & Thread_running) ? 8 : 0xd;
  else if (s & (Thread_waiting | Thread_receiving)) 
    s = (s & Thread_running) ? 8 : 0xc;
  else
    s = 0;

  param->prio (timesharing_sched()->prio());
  param->time (timesharing_sched()->timeslice() * Config::microsec_per_tick);
  param->thread_state (s);
}

/*
 * Set normal timesharing parameters
 */
PRIVATE
void
Thread::set_timesharing_param (L4_sched_param param)
{
  if (EXPECT_FALSE
     (param.prio() > thread_lock()->lock_owner()->mcp() ||
      param.time() == (Unsigned64) -1))
    return;

  if (timesharing_sched()->prio() != param.prio())
    {
      // We need to protect the priority manipulation so that
      // this thread cannot be preempted and ready-enqueued
      // according to a wrong priority

      Lock_guard<Cpu_lock> guard (&cpu_lock);

      if (in_ready_list())	// need to re-queue in ready queue
        ready_dequeue();	// according to new prio

      timesharing_sched()->set_prio (param.prio());

      // ready_enqueue happens during thread_lock release
    }

  set_small_space (param.small());

  timesharing_sched()->set_timeslice (round_timeslice (param.time()));
}

/*
 * Set preempter unless "invalid"
 */
PRIVATE
void
Thread::set_preempter (L4_uid preempter)
{
  if (EXPECT_FALSE
     (preempter.is_invalid()))
    return;

  Thread *p = Threadid (preempter).lookup();

  if (p && p->exists())
    _ext_preempter = p;
}

/*
 * Add a realtime timeslice at the end of the list
 */
PRIVATE
void
Thread::set_realtime_param (L4_sched_param param)
{
  if (EXPECT_FALSE
     (param.prio() > thread_lock()->lock_owner()->mcp() ||
      param.time() == (Unsigned64) -1))
    return;

  Sched_context *s = new Sched_context (this,
                                        timesharing_sched()->prev()->id() + 1,
                                        param.prio(),
                                        round_timeslice (param.time()));

  s->enqueue_before (timesharing_sched());
}

/*
 * Remove all realtime timeslices
 */
PRIVATE
void
Thread::remove_realtime_param()
{
  Sched_context *s, *tmp;

  for (s  = timesharing_sched()->next();
       s != timesharing_sched();
       tmp = s, s = s->next(), tmp->dequeue(), delete tmp);
}

PRIVATE
void
Thread::begin_period (Unsigned64 clock)
{
  _sched_timeout.set (clock);
}

PRIVATE
void
Thread::end_period()
{}

/*
 * L4 system call thread_schedule.
 */
IMPLEMENT inline NOEXPORT
void
Thread::sys_thread_schedule()
{
  Sys_thread_schedule_frame *regs = sys_frame_cast<Sys_thread_schedule_frame>(this->regs());
  Thread *dst;

  // Lookup destination thread
  if (!(dst = Threadid (regs->dest()).lookup()))
    {
      regs->old_param ((Mword) -1);	// Return error
      return;
    }

  Lock_guard <Thread_lock> guard (dst->thread_lock());

  // Check that thread exists and prio doesn't exceed my MCP
  // XXX: Should we check sched or timesharing_sched against MCP here?
  if (!dst->exists() || dst->timesharing_sched()->prio() > mcp())
    {
      regs->old_param ((Mword) -1);	// Return error
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
          case 0:			// get/set timesharing parameters
            dst->set_timesharing_param (regs->param());
            break;

          case 1:			// set realtime parameters
            dst->set_realtime_param (regs->param());
            return;

          case 2:			// remove realtime timeslices
            dst->remove_realtime_param();
            return;

          case 3:			// begin_period
            dst->begin_period (regs->time());
            return;

          case 4:			// end_period
            dst->end_period();
            return;

          default:
            kdb_ke ("thread_schedule: param_word bits 16-19 must be 0. Fix your bindings!");
            return;
        }
    }

  // Set preempter
  dst->set_preempter (regs->preempter());

  // Setup return parameters
  regs->old_param     (param);
  regs->old_preempter (preempter);
  regs->partner       (partner);
  regs->time          (dst->timesharing_sched()->get_total_cputime());
}

extern "C" void sys_thread_schedule_wrapper()
{
  current_thread()->sys_thread_schedule();
}
