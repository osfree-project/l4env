/**
 * \file  roottask/lib/src/libroot.c
 */

#include <l4/sys/syscalls.h>
#include <l4/rmgr/proto.h>
#include <l4/rmgr/librmgr.h>
#ifdef USE_TASKLIB
#include <l4/roottask/rmgr_task-client.h>
#else
#include <l4/roottask/rmgr-client.h>
#endif

/* XXX Check return value of DICE environment (e.g. IPC errors!) */

l4_threadid_t rmgr_id;
l4_threadid_t rmgr_pager_id;

/**
 * Find the roottask.
 * \return    0 on ERROR, != 0 on SUCCESS
 */
int
rmgr_init(void)
{
  DICE_DECLARE_ENV(env);
  long value;

  /* Doing l4_nchief(L4_INVALID_ID, &my_pager); is not possible when
   * roottask is not the chief of all programs. Every chief would need to
   * implement the roottask protocol or at least a query_roottask function
   */
  rmgr_id.id.task          = rmgr_pager_id.id.task = RMGR_TASK_ID;
  rmgr_id.id.lthread       = RMGR_LTHREAD_SUPER;
  rmgr_pager_id.id.lthread = RMGR_LTHREAD_PAGER;

  rmgr_init_ping_call(&rmgr_id, 0xaffeaffe, &value, &env);

  return value == ~0xaffeaffeUL;
}

/**
 * Move a task into an allocated small space.
 * \param  tid  ID of corresponding L4 task
 * \param  num  number of small space
 * \return 0    on success, 1 otherwise
 */
int
rmgr_set_small_space(l4_threadid_t tid, int num)
{
  DICE_DECLARE_ENV(env);
  return rmgr_set_small_space_call(&rmgr_id, &tid, num, &env);
}

/**
 * Set priority of a thread.
 * \param  tid  target thread ID
 * \param  prio new thread priority
 * \return 0    on success, 1 otherwise
 */
int
rmgr_set_prio(l4_threadid_t tid, int prio)
{
  static short mcp_below = 0xff;
  DICE_DECLARE_ENV(env);

  /* try to set directly first */
  if (prio <= mcp_below)
    {
      l4_threadid_t foo_id = L4_INVALID_ID;
      l4_sched_param_t schedparam;

      l4_thread_schedule(tid, L4_INVALID_SCHED_PARAM,
			 &foo_id, &foo_id, &schedparam);
      if (!l4_is_invalid_sched_param(schedparam))
	{
	  schedparam.sp.prio = prio;
	  foo_id = L4_INVALID_ID;
	  l4_thread_schedule(tid, schedparam, &foo_id, &foo_id, &schedparam);
	  if (!l4_is_invalid_sched_param(schedparam))
	    {
	      /* the spec is a bit unprecise here: the syscall returns no
	       * error if the *old* prio was below mcp. If new prio was
	       * not>=mcp, no error will be returned, but the prio won't be
	       * set. */
	      l4_thread_schedule(tid, L4_INVALID_SCHED_PARAM,
                                 &foo_id, &foo_id, &schedparam);
	      if (!l4_is_invalid_sched_param(schedparam) &&
	          schedparam.sp.prio == prio)
		return 0;
	    }
	  mcp_below = prio - 1;
	  if (mcp_below < 0)
	    mcp_below = 0;
	}
      else
	/* even failed to query the schedparam word... */
	mcp_below = 0;
    }

  /* failed to set directly -- use the RMGR */
  return rmgr_set_prio_call(&rmgr_id, &tid, prio, &env);
}

/**
 * Query prio without rmgr involvement.
 * \param  tid  target thread ID
 * \retval prio priority
 * \return 0    on success, 1 otherwise
 */
int
rmgr_get_prio(l4_threadid_t tid, int *prio)
{
  l4_sched_param_t p;
  l4_threadid_t foo_id = L4_INVALID_ID;

  l4_thread_schedule(tid, L4_INVALID_SCHED_PARAM,
                     &foo_id, &foo_id, &p);

  return *prio = l4_is_invalid_sched_param(p) ? 0 : p.sp.prio;
}

/**
 * Request roottask to transfer right for a task to the caller.
 * \param  num  task number
 * \return 0    on success, 1 otherwise
 */
int
rmgr_get_task(int num)
{
  DICE_DECLARE_ENV(env);
  return rmgr_get_task_call(&rmgr_id, num, &env);
}

/**
 * Pass right for a task back to roottask.
 * \param num  task number
 * \return 0    on success, 1 otherwise
 */
int
rmgr_free_task(int num)
{
  l4_threadid_t n;
  DICE_DECLARE_ENV(env);

  /* Give task creation right back to RMGR */
  n.id.task = num;
  l4_task_new(n, (l4_umword_t)rmgr_id.raw, /* lower part, if any (x0...) */
              0, 0, L4_NIL_ID);

  /* And then tell RMGR about it */
  return rmgr_free_task_call(&rmgr_id, num, &env);
}

/**
 * Free all L4 tasks occupied for a specific task.
 * \param  tid  target task ID
 * \return 0    on success, 1 otherwise
 */
int
rmgr_free_task_all(l4_threadid_t tid)
{
  DICE_DECLARE_ENV(env);
  return rmgr_free_task_all_call(&rmgr_id, &tid, &env);
}

/**
 * \return 0	on success, 1 otherwise
 */
int
rmgr_get_irq(int num)
{
  DICE_DECLARE_ENV(env);
  return rmgr_get_irq_call(&rmgr_id, num, &env);
}

/**
 * \return 0	on success, 1 otherwise
 */
int
rmgr_free_irq(int num)
{
  DICE_DECLARE_ENV(env);
  return rmgr_free_irq_call(&rmgr_id, num, &env);
}

/**
 * Free all IRQs occupied for a task.
 * \return 0    on success
 */
int
rmgr_free_irq_all(l4_threadid_t tid)
{
  DICE_DECLARE_ENV(env);
  return rmgr_free_irq_all_call(&rmgr_id, &tid, &env);
}

/**
 * \return 0    on success, 1 otherwise
 */
int
rmgr_free_fpage(l4_fpage_t fp)
{
  DICE_DECLARE_ENV(env);
  return rmgr_free_fpage_call(&rmgr_id, fp.raw, &env);
}

/**
 * \return 0    on success, 1 otherwise
 */
int
rmgr_free_page(l4_umword_t addr)
{
  DICE_DECLARE_ENV(env);
  return rmgr_free_page_call(&rmgr_id, addr, &env);
}

/**
 * \return 0    on success
 */
int
rmgr_dump_mem(void)
{
  DICE_DECLARE_ENV(env);
  return rmgr_dump_mem_call(&rmgr_id, &env);
}

/**
 * \return ~0U  if no area was found
 */
l4_umword_t
rmgr_reserve_mem(l4_umword_t size, l4_umword_t align, l4_umword_t flags,
		 l4_umword_t range_low, l4_umword_t range_high)
{
  l4_addr_t addr;
  DICE_DECLARE_ENV(env);

  if (rmgr_reserve_mem_call(&rmgr_id, size, align, flags,
                            range_low, range_high, &addr, &env))
    return addr;

  return ~0U;
}

/**
 * Free all memory occupied by the target task.
 * \return  0   on success
 */
int
rmgr_free_mem_all(l4_threadid_t tid)
{
  DICE_DECLARE_ENV(env);
  return rmgr_free_mem_all_call(&rmgr_id, &tid, &env);
}

/**
 * Request the first physical page (roottask does not hand out this page on
 * pagefaults).
 */
int
rmgr_get_page0(void *address)
{
  l4_snd_fpage_t page0;
  DICE_DECLARE_ENV(env);

  env.rcv_fpage = l4_fpage((l4_umword_t)address, L4_LOG2_PAGESIZE,
                           L4_FPAGE_RO, L4_FPAGE_MAP);

  rmgr_get_page0_call(&rmgr_id, &page0, &env);

  return 0;
}

/**
 * Request the task ID of the boot module named by modname.
 * \return 0    on success, 1 otherwise
 */
int
rmgr_get_task_id(const char *modname, l4_threadid_t *tid)
{
  DICE_DECLARE_ENV(env);
  return rmgr_get_task_id_call(&rmgr_id, modname, tid, &env);
}

/**
 * Specify the name of a boot module.
 * \return 0    on success, 1 otherwise
 */
int
rmgr_set_task_id(const char *modname, l4_threadid_t tid)
{
  DICE_DECLARE_ENV(env);
  return rmgr_set_task_id_call(&rmgr_id, modname, &tid, &env);
}

/**
 * Create an L4 task.
 * \return a valid task ID on success, L4_NIL_ID otherwise
 */
l4_taskid_t
rmgr_task_new(l4_taskid_t tid, l4_umword_t mcp_or_chief,
              l4_umword_t esp, l4_umword_t eip, l4_threadid_t pager)
{
  DICE_DECLARE_ENV(env);
  l4_threadid_t ntid = L4_NIL_ID;
  l4_threadid_t inv = L4_INVALID_ID;
  l4_quota_desc_t q = L4_INVALID_KQUOTA;

  if (rmgr_task_new_call(&rmgr_id, &tid, mcp_or_chief, esp, eip,
                         &pager, &inv, &q, -1, &ntid, &env))
    return L4_NIL_ID;

  return ntid;
}

/**
 * Create an L4 task with a capability handler.
 * \return a valid task ID on success, L4_NIL_ID otherwise
 */
l4_taskid_t
rmgr_task_new_long(l4_taskid_t tid, l4_umword_t mcp_or_chief,
                   l4_umword_t esp, l4_umword_t eip,
                   l4_threadid_t pager, l4_threadid_t caphandler,
                   l4_quota_desc_t kquota)
{
  DICE_DECLARE_ENV(env);
  l4_threadid_t ntid = L4_NIL_ID;

  if (rmgr_task_new_call(&rmgr_id, &tid, mcp_or_chief, esp, eip,
                         &pager, &caphandler, &kquota, -1, &ntid, &env))
    return L4_NIL_ID;

  return ntid;
}

/**
 * Create an L4 task with an speciic prioriy.
 * \return a valid task ID on success, L4_NIL_ID otherwise
 */
l4_taskid_t
rmgr_task_new_with_prio(l4_taskid_t tid, l4_umword_t mcp_or_chief,
			l4_umword_t esp, l4_umword_t eip, l4_threadid_t pager,
			l4_sched_param_t sched_param)
{
  DICE_DECLARE_ENV(env);
  l4_threadid_t ntid = L4_NIL_ID;
  l4_threadid_t inv = L4_INVALID_ID;
  l4_quota_desc_t q = L4_INVALID_KQUOTA;

  if (rmgr_task_new_call(&rmgr_id, &tid, mcp_or_chief, esp, eip,
                         &pager, &inv, &q, sched_param.sched_param, &ntid,
                         &env))
    return L4_NIL_ID;

  return ntid;
}

int
rmgr_privctrl(l4_umword_t cmd, l4_umword_t param)
{
  DICE_DECLARE_ENV(env);
  return rmgr_privctrl_call(&rmgr_id, cmd, param, &env);
}
