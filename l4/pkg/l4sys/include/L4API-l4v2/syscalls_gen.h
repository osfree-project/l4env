#ifndef __L4_SYS__L4API_l4v2__SYSCALLS_GEN_H__
#define __L4_SYS__L4API_l4v2__SYSCALLS_GEN_H__

/**
 * Flags for l4_task_new
 * \ingroup api_calls_other
 */
enum {
  L4_TASK_NEW_RAISE_EXCEPTION = 1UL << 30,    /**< Raise exception upon start of thread 0 */
  L4_TASK_NEW_ALIEN           = 1UL << 31,    /**< Start thread 0 in alien mode */
};

/**
 * Internal flags for l4_task_new
 * \internal
 * \ingroup api_calls_other
 */
enum {
  L4_TASK_NEW_UTCB_ARGS       = 1UL << 29,   /**< Additional arguments in UTCB */
  L4_TASK_NEW_NR_OF_FLAGS     = 3,           /**< Number of flags */
  /** Mask of all flags */
  L4_TASK_NEW_FLAGS_MASK      = ((1 << L4_TASK_NEW_NR_OF_FLAGS) - 1)
                                << (32 - L4_TASK_NEW_NR_OF_FLAGS),
};

/**
 * \anchor l4_thread_ex_regs_flags_options
 * Flags for l4_thread_ex_regs
 * \ingroup api_calls_other
 */
enum {
  /** The target thread will raise an exception immediately just before
   * returning to userland. */
  L4_THREAD_EX_REGS_RAISE_EXCEPTION = 1UL << 28,
  /** The target thread is handled as alien, that is every L4 syscall
   * will generate an exception IPC. */
  L4_THREAD_EX_REGS_ALIEN           = 1UL << 29,
  /** Do not cancel ongoing IPC. */
  L4_THREAD_EX_REGS_NO_CANCEL       = 1UL << 30,

  /** Offset of task number in bits */
  L4_THREAD_EX_REGS_TASK_ID_SHIFT   = 7,
};

/**
 * Internal flags for l4_thread_ex_regs
 * \internal
 * \ingroup api_calls_other
 */
enum {
  /** Additional arguments are delivered in the UTCB */
  L4_THREAD_EX_REGS_UTCB_ARGS       = 1UL << 27,
  /** Reserved for backward compatibility with older L4 versions. */
  L4_THREAD_EX_REGS_VM86            = 1UL << 31,
};


/*---------------------------------------------------------------------
 * Implementations
 *--------------------------------------------------------------------*/

static inline l4_umword_t
l4_thread_ex_regs_reg0(l4_umword_t threadnr,
                       l4_umword_t tasknr,
                       l4_umword_t flags)
{
  return threadnr | (tasknr << L4_THREAD_EX_REGS_TASK_ID_SHIFT) | flags;
}

/*
 * L4 lthread_ex_regs
 */
L4_INLINE void
l4_thread_ex_regs(l4_threadid_t destination,
		  l4_umword_t eip,
		  l4_umword_t esp,
		  l4_threadid_t *preempter,
		  l4_threadid_t *pager,
		  l4_umword_t *old_eflags,
		  l4_umword_t *old_eip,
		  l4_umword_t *old_esp)
{
  l4_thread_ex_regs_sc(destination.id.lthread,
                       eip, esp, preempter, pager,
                       old_eflags, old_eip, old_esp);
}

/*
 * L4 lthread_ex_regs with additional flags
 */
L4_INLINE void
l4_thread_ex_regs_flags(l4_threadid_t destination,
                        l4_umword_t eip,
                        l4_umword_t esp,
                        l4_threadid_t *preempter,
                        l4_threadid_t *pager,
                        l4_umword_t *old_eflags,
                        l4_umword_t *old_eip,
                        l4_umword_t *old_esp,
                        unsigned long flags)
{
  l4_thread_ex_regs_sc(l4_thread_ex_regs_reg0(destination.id.lthread, 0, flags),
                       eip, esp, preempter, pager,
                       old_eflags, old_eip, old_esp);
}

/*
 * L4 lthread_inter_task_ex_regs
 */
L4_INLINE void
l4_inter_task_ex_regs(l4_threadid_t destination,
                      l4_umword_t eip,
                      l4_umword_t esp,
                      l4_threadid_t *preempter,
                      l4_threadid_t *pager,
                      l4_threadid_t *cap_handler,
                      l4_umword_t *old_eflags,
                      l4_umword_t *old_eip,
                      l4_umword_t *old_esp,
                      unsigned long flags,
                      l4_utcb_t *utcb)
{
  utcb->ex_regs.caphandler = *cap_handler;
  l4_thread_ex_regs_sc(l4_thread_ex_regs_reg0(destination.id.lthread,
                                              destination.id.task,
                                              flags | L4_THREAD_EX_REGS_UTCB_ARGS),
                       eip, esp, preempter, pager,
                       old_eflags, old_eip, old_esp);
  *cap_handler = utcb->ex_regs.caphandler;
}

/*
 * Special version of l4_thread_ex_regs to get the pager of a thread
 */
L4_INLINE l4_threadid_t
l4_thread_ex_regs_pager(l4_threadid_t destination)
{
  l4_umword_t dummy;
  l4_threadid_t preempter = L4_INVALID_ID;
  l4_threadid_t pager     = L4_INVALID_ID;

  l4_thread_ex_regs_flags(destination, ~0UL, ~0UL,
                          &preempter, &pager, &dummy, &dummy, &dummy,
                          L4_THREAD_EX_REGS_NO_CANCEL);
  return pager;
}

/* ================ l4_task_new variants =================== */


L4_INLINE l4_taskid_t
l4_task_new(l4_taskid_t destination,
            l4_umword_t mcp_or_new_chief_and_flags,
            l4_umword_t esp,
            l4_umword_t eip,
            l4_threadid_t pager)
{
  return l4_task_new_sc(destination, mcp_or_new_chief_and_flags,
                        esp, eip, pager);
}

/*
 * L4 task new with cap
 */
L4_INLINE l4_taskid_t
l4_task_new_long(l4_taskid_t destination,
                 l4_umword_t mcp_or_new_chief_and_flags,
                 l4_umword_t esp,
                 l4_umword_t eip,
                 l4_threadid_t pager,
                 l4_threadid_t cap_handler,
                 l4_quota_desc_t kquota,
                 l4_utcb_t *utcb)
{
  utcb->task_new.caphandler = cap_handler;
  utcb->task_new.quota      = kquota;
  return l4_task_new_sc(destination,
                        mcp_or_new_chief_and_flags | L4_TASK_NEW_UTCB_ARGS,
                        esp, eip, pager);
}

L4_INLINE void
l4_yield(void)
{
  l4_thread_switch(L4_NIL_ID);
}

#endif /* ! __L4_SYS__L4API_l4v2__SYSCALLS_GEN_H__ */
