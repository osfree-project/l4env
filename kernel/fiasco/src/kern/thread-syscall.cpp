
INTERFACE:
class Entry_frame;

extern unsigned (Thread::*syscall_table[])(Syscall_frame *regs);

IMPLEMENTATION [syscall]:

#include <cstdio>

#include "l4_types.h"

#include "config.h"
#include "space.h"
#include "space_index_util.h"
#include "map_util.h"
#include "thread_util.h"
#include "logdefs.h"

Mword (Thread::*syscall_table[])(Syscall_frame *regs) = 
{ 
  &Thread::sys_ipc, 
  &Thread::sys_id_nearest, 
  &Thread::sys_fpage_unmap, 
  &Thread::sys_thread_switch, 
  &Thread::sys_thread_schedule, 
  &Thread::sys_thread_ex_regs, 
  &Thread::sys_task_new
};


extern "C" void some_very_big_cast_trouble();

/*
 * Wrapper for all system calls, callable from assembler routines
 */
extern "C" void dispatch_syscall (Thread *t, Entry_frame *regs, 
				   unsigned int number)
{
  unsigned result;

  t->state_change_dirty(~Thread_cancel, 0);
  Proc::sti();

  if (regs != static_cast<Syscall_frame*>(regs))
    some_very_big_cast_trouble();
  if (number > 6)
    printf("syscall number is %d\n", number);
  result = (t->*(syscall_table[number]))(static_cast<Syscall_frame *>(regs));

#ifdef CONFIG_IA32
  regs->eax = result;
#endif
  Proc::cli();
}



/** L4 system call lthread_ex_regs.  */
IMPLEMENT
Mword Thread::sys_thread_ex_regs(Syscall_frame *i_regs)
{
  Sys_ex_regs_frame *regs = static_cast<Sys_ex_regs_frame*>(i_regs);

  L4_uid dst_id_long = id();
  dst_id_long.lthread( regs->lthread() );
  threadid_t dst_id(&dst_id_long);

  Thread *dst = dst_id.lookup();

  bool new_thread = false;

  if (! dst->exists())
    {
      new_thread = true;
      check (new (dst_id) Thread (space(), dst_id_long, 
				  sched()->prio(), sched()->mcp())
	     == dst);
    }

  Thread *o_pager, *o_preempter;
  vm_offset_t o_flags;
 
  LOG_THREAD_EX_REGS;

  // initialize the thread with new values.

  // undocumented but jochen-compatible: if we're creating a new
  // thread (and aren't modifying an existing one), don't change the
  // register parameters that have been passed to us to ``old'' values
  // (i.e., all zeroes)
  Mword o_ip, o_sp;
  if (dst->initialize(regs->ip(), regs->sp(), 
		      threadid_t(regs->pager()).lookup(),
		      threadid_t(regs->preempter()).lookup(),
		      new_thread ? 0 : & o_ip, & o_sp, 
		      new_thread ? 0 : & o_pager, 
		      new_thread ? 0 : & o_preempter, 
		      new_thread ? 0 : & o_flags))
    {
      if (new_thread) return 0;

      regs->old_sp(o_sp);
      regs->old_ip(o_ip);
      regs->old_pager( o_pager ? o_pager->id() : L4_uid::INVALID );
      regs->old_preempter( o_preempter ? o_preempter->id() : L4_uid::INVALID );

      return o_flags;
    }

  return (Mword)-1;		// error
}

/** L4 system call fpage_unmap.  */
IMPLEMENT
Mword Thread::sys_fpage_unmap(Syscall_frame *i_regs)
{
  Sys_unmap_frame *regs = static_cast<Sys_unmap_frame*>(i_regs);
  fpage_unmap(space(), regs->fpage(),
	      regs->self_unmap(), regs->downgrade());
  return 0;
}

/** L4 system call thread_switch.  */
IMPLEMENT
Mword
Thread::sys_thread_switch(Syscall_frame *i_regs)
{
  Sys_thread_switch_frame *regs = static_cast<Sys_thread_switch_frame*>(i_regs);
  // If someone calls thread_switch(L4_NIL_ID) we have to schedule
  // within the context of the calling thread. Otherwise we may end up
  // in an endless loop.  
  // (idle thread -> thread calling thread_switch -> other ready threads)
  // Therefore we have to check for L4_NIL_ID and can't rely on the
  // idle thread to do a reschedule for us.

  Thread *t = threadid_t(regs->dest()).lookup();
  if (t && regs->has_dest() && (t->state() & Thread_running))
    switch_to(t);
  else
    {
      timeslice_ticks_left = 0;
      schedule();
    }
  return 0;
}

/** L4 system call thread_schedule.  */
IMPLEMENT
Mword Thread::sys_thread_schedule(Syscall_frame *i_regs)
{
  Sys_thread_schedule_frame *regs = static_cast<Sys_thread_schedule_frame*>(i_regs);

  Thread *dst = threadid_t(regs->dest()).lookup();

  if (!dst || dst->state() == Thread_invalid)
    return (Mword)-1;		// error

  L4_sched_param s;
  Thread *opre;
  Sender *partner;

  Unsigned64 time;

  if (! dst->set_sched_param(regs->param(),
			     threadid_t(regs->preempter()).lookup(),
			     &s, &time,
			     &opre, &partner))
    return (Mword)-1;

  regs->time(time);
  regs->partner(partner ? partner->id() : L4_uid::INVALID);
  regs->old_preempter( opre ? opre->id() : L4_uid::INVALID);

  return s.raw();
}


/** L4 system call id_nearest.  */
IMPLEMENT
Mword Thread::sys_id_nearest(Syscall_frame *i_regs)
{
  enum {
    L4_NC_SAME_CLAN  = 0x00,	/* destination resides within the */
                                /* same clan */
    L4_NC_INNER_CLAN = 0x0C,	/* destination is in an inner clan */
    L4_NC_OUTER_CLAN = 0x04,	/* destination is outside the */
				/* invoker's clan */
  };

  Sys_id_nearest_frame *regs = static_cast<Sys_id_nearest_frame*>(i_regs);
  L4_uid dst_id_long = regs->dest();
  threadid_t dst_id(&dst_id_long);

  if (dst_id.is_nil())
    {
      regs->nearest( id() );
      return L4_NC_SAME_CLAN;
    }

  // special case for undocumented Jochen-compatibility: if id ==
  // invalid_id, deliver our chief
  if (! dst_id.is_valid())
    {
      regs->nearest( lookup_first_thread(chief_index())->id() );
      return L4_NC_OUTER_CLAN;
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
  return ret;

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
  return L4_IPC_ENOT_EXISTENT;
#endif
}

