IMPLEMENTATION[syscall-v2x0]:

void (*syscall_table[])() = 
{ 
  sys_ipc_wrapper,
  sys_id_nearest_wrapper,
  sys_fpage_unmap_wrapper,
  sys_thread_switch_wrapper,
  sys_thread_schedule_wrapper,
  sys_thread_ex_regs_wrapper,
  sys_task_new_wrapper
};

/** L4 system call lthread_ex_regs.  */
IMPLEMENT inline NOEXPORT
void Thread::sys_thread_ex_regs()
{
  Sys_ex_regs_frame *regs = sys_frame_cast<Sys_ex_regs_frame>(this->regs());

  L4_uid dst_id_long = id();
  dst_id_long.lthread( regs->lthread() );
  Threadid dst_id(&dst_id_long);

  Thread *dst = dst_id.lookup();

  bool new_thread = false;

  if (! dst->exists())
    {
      new_thread = true;
      check (new (dst_id) Thread (space(), dst_id_long, 
				  sched()->prio(), mcp())
	     == dst);
    }

  // o_pager and o_preempter don't need to be initialized since they are
  // only evaluated if new_thread is false
  Thread *o_pager, *o_preempter;
  Address o_flags;

  LOG_THREAD_EX_REGS;

  // initialize the thread with new values.

  // undocumented but jochen-compatible: if we're creating a new
  // thread (and aren't modifying an existing one), don't change the
  // register parameters that have been passed to us to ``old'' values
  // (i.e., all zeroes)
  Mword o_ip, o_sp;
  if (dst->initialize(regs->ip(), regs->sp(), 
		      Threadid(regs->pager()).lookup(),
		      Threadid(regs->preempter()).lookup(),
		      new_thread ? 0 : & o_ip, & o_sp, 
		      new_thread ? 0 : & o_pager, 
		      new_thread ? 0 : & o_preempter, 
		      new_thread ? 0 : & o_flags))
    {
      if (new_thread)
	{
	  regs->old_eflags(0);
	  return;
	}

      regs->old_sp(o_sp);
      regs->old_ip(o_ip);
      regs->old_pager( o_pager ? o_pager->id() : L4_uid::INVALID );
      regs->old_preempter( o_preempter ? o_preempter->id() : L4_uid::INVALID );
      regs->old_eflags(o_flags);
      return;
    }

  regs->old_eflags ((Mword) -1);	// error
}

/** L4 system call id_nearest.  */
IMPLEMENT inline NOEXPORT
void Thread::sys_id_nearest()
{
  enum {
    L4_NC_SAME_CLAN  = 0x00,	/* destination resides within the */
                                /* same clan */
    L4_NC_INNER_CLAN = 0x0C,	/* destination is in an inner clan */
    L4_NC_OUTER_CLAN = 0x04,	/* destination is outside the */
				/* invoker's clan */
  };

  Sys_id_nearest_frame *regs = sys_frame_cast<Sys_id_nearest_frame>(this->regs());
  L4_uid dst_id_long = regs->dest();
  Threadid dst_id(&dst_id_long);

  if (dst_id.is_nil())
    {
      regs->nearest( id() );
      regs->type (L4_NC_SAME_CLAN);
      return;
    }

  // special case for undocumented Jochen-compatibility: if id ==
  // invalid_id, deliver our chief
  if (! dst_id.is_valid())
    {
      regs->nearest( lookup_first_thread(chief_index())->id() );
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
  return L4_IPC_ENOT_EXISTENT;
#endif
}

extern "C" void sys_id_nearest_wrapper()
{
  current_thread()->sys_id_nearest();
}

extern "C" void sys_thread_ex_regs_wrapper()
{
  current_thread()->sys_thread_ex_regs();
}

