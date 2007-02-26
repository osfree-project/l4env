
#include <l4/sys/syscalls.h>
#include <l4/rmgr/proto.h>
#include <l4/rmgr/librmgr.h>
#include <l4/roottask/rmgr-client.h>

/* XXX Check return value of DICE environment (e.g. IPC errors!) */

l4_threadid_t rmgr_id;
l4_threadid_t rmgr_pager_id;

int
rmgr_init(void)
{
  l4_threadid_t my_pager;
  DICE_DECLARE_ENV(env);
  l4_int32_t ret, value;

  l4_nchief(L4_INVALID_ID, &my_pager);

  rmgr_id = rmgr_pager_id  = my_pager;
  rmgr_id.id.lthread       = RMGR_LTHREAD_SUPER;
  rmgr_pager_id.id.lthread = RMGR_LTHREAD_PAGER;

  ret = rmgr_init_ping_call(&rmgr_id, 0xaffeaffe, &value, &env);

  return ret == 0 && value == ~0xaffeaffe;
}

int
rmgr_set_small_space(l4_threadid_t tid, int num)
{
  DICE_DECLARE_ENV(env);
  return rmgr_set_small_space_call(&rmgr_id, &tid, num, &env);
}

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

/** Query prio without rmgr involvement. */
int
rmgr_get_prio(l4_threadid_t tid, int *prio)
{
  l4_sched_param_t p;
  l4_threadid_t foo_id = L4_INVALID_ID;

  l4_thread_schedule(tid, L4_INVALID_SCHED_PARAM,
                     &foo_id, &foo_id, &p);

  return *prio = l4_is_invalid_sched_param(p) ? 0 : p.sp.prio;
}


int
rmgr_get_task(int num)
{
  DICE_DECLARE_ENV(env);
  return rmgr_get_task_call(&rmgr_id, num, &env);
}

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

int
rmgr_free_task_all(l4_threadid_t tid)
{
  DICE_DECLARE_ENV(env);
  return rmgr_free_task_all_call(&rmgr_id, &tid, &env);
}

/**
 * \retval 0	Success
 * \retval 1	Error
 */
int
rmgr_get_irq(int num)
{
  DICE_DECLARE_ENV(env);
  return rmgr_get_irq_call(&rmgr_id, num, &env);
}

/**
 * \retval 0	Success
 * \retval 1	Error
 */
int
rmgr_free_irq(int num)
{
  DICE_DECLARE_ENV(env);
  return rmgr_free_irq_call(&rmgr_id, num, &env);
}

int
rmgr_free_irq_all(l4_threadid_t tid)
{
  DICE_DECLARE_ENV(env);
  return rmgr_free_irq_all_call(&rmgr_id, &tid, &env);
}

int
rmgr_free_fpage(l4_fpage_t fp)
{
  DICE_DECLARE_ENV(env);
  return rmgr_free_fpage_call(&rmgr_id, fp.raw, &env);
}

int
rmgr_free_page(l4_umword_t addr)
{
  DICE_DECLARE_ENV(env);
  return rmgr_free_page_call(&rmgr_id, addr, &env);
}

int
rmgr_dump_mem(void)
{
  DICE_DECLARE_ENV(env);
  return rmgr_dump_mem_call(&rmgr_id, &env);
}

l4_umword_t
rmgr_reserve_mem(l4_umword_t size, l4_umword_t align, l4_umword_t flags,
		 l4_umword_t range_low, l4_umword_t range_high)
{
  l4_addr_t addr;
  DICE_DECLARE_ENV(env);

  if (rmgr_reserve_mem_call(&rmgr_id, size, align, flags,
                            range_low, range_high, &addr, &env))
    return addr;

  return 0;
}

int
rmgr_free_mem_all(l4_threadid_t tid)
{
  DICE_DECLARE_ENV(env);
  return rmgr_free_mem_all_call(&rmgr_id, &tid, &env);
}

int
rmgr_get_page0(void *address)
{
  l4_snd_fpage_t page0;
  DICE_DECLARE_ENV(env);

  env.rcv_fpage = l4_fpage((l4_umword_t)address, L4_LOG2_PAGESIZE,
                           L4_FPAGE_RO, L4_FPAGE_MAP);

  return rmgr_get_page0_call(&rmgr_id, &page0, &env);
}

int
rmgr_get_task_id(const char *modname, l4_threadid_t *tid)
{
  DICE_DECLARE_ENV(env);
  return rmgr_get_task_id_call(&rmgr_id, modname, tid, &env);
}

int
rmgr_set_task_id(const char *modname, l4_threadid_t tid)
{
  DICE_DECLARE_ENV(env);
  return rmgr_get_task_id_call(&rmgr_id, modname, &tid, &env);
}

l4_taskid_t
rmgr_task_new(l4_taskid_t tid, l4_umword_t mcp_or_chief,
	      l4_umword_t esp, l4_umword_t eip, l4_threadid_t pager)
{
  DICE_DECLARE_ENV(env);
  l4_threadid_t ntid;

  if (rmgr_task_new_call(&rmgr_id, &tid, mcp_or_chief, esp, eip,
                         &pager, -1, &ntid, &env))
    return L4_NIL_ID;

  return ntid;
}

l4_taskid_t
rmgr_task_new_with_prio(l4_taskid_t tid, l4_umword_t mcp_or_chief,
			l4_umword_t esp, l4_umword_t eip, l4_threadid_t pager,
			l4_sched_param_t sched_param)
{
  DICE_DECLARE_ENV(env);
  l4_threadid_t ntid;

  if (rmgr_task_new_call(&rmgr_id, &tid, mcp_or_chief, esp, eip,
                         &pager, sched_param.sched_param, &ntid, &env))
    return L4_NIL_ID;

  return ntid;
}

int
rmgr_privctrl(l4_umword_t cmd, l4_umword_t param)
{
  DICE_DECLARE_ENV(env);
  return rmgr_privctrl_call(&rmgr_id, cmd, param, &env);
}
