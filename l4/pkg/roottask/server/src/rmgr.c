#include <stdio.h>
#include <unistd.h>

#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/util/l4_macros.h>
#include <l4/rmgr/librmgr.h>
#include <l4/rmgr/proto.h>
#include <l4/env_support/panic.h>
#include <l4/env_support/getchar.h>

#include "exec.h"
#include "globals.h"
#include "memmap.h"
#include "memmap_lock.h"
#include "names.h"
#include "rmgr.h"
#include "pager.h"
#include "irq.h"
#include "region.h"
#include "small.h"
#include "task.h"
#include "iomap.h"
#include "quota.h"
#include "rmgr-server.h"


#define MGR_STACKSIZE 8192

l4_threadid_t myself;		/* used to derive taskids for new tasks */
l4_threadid_t my_pager;
l4_threadid_t my_preempter;
l4_threadid_t rmgr_super_id;	/* resource manager normally thread (4.1)*/
l4_threadid_t rmgr_pager_id;	/* pager normally thread (4.0) */
int           l4_version;
int           ux_running;
int           quiet;

static char mgr_stack[MGR_STACKSIZE] __attribute__((aligned(8)));
static void mgr(void) L4_NORETURN;

l4_addr_t mem_lower;
l4_addr_t mem_upper;

unsigned small_space_size;

/* logging facility */
unsigned debug_log_mask = 0;
unsigned debug_log_types = 0;
static unsigned do_log = 0;

/**
 * Free unused memory.
 */
static void
free_init_section(void)
{
  l4_addr_t address;
  const l4_addr_t free_beg = l4_trunc_page(&_start);
  const l4_addr_t free_end = l4_trunc_page(&_stext);

  /* free .init section */
  for (address  = free_beg; address < free_end; address += L4_PAGESIZE)
    {
      memset((void*)address, 0, L4_PAGESIZE);
      if (!memmap_free_page(address, O_RESERVED))
	printf ("ROOT: Error freeing .init at %08lx\n", address);
    }

  region_free(free_beg, free_end);
}

void
rmgr_main(int memdump)
{
  l4_threadid_t pa, pr;
  l4_umword_t dummy;

  /* free the .init section */
  free_init_section();

  if (memdump)
    {
      memmap_dump();
      putchar('\n');
    }

  pa = my_pager;
  pr = my_preempter;

  l4_thread_ex_regs(rmgr_super_id, (l4_umword_t) mgr,
		    (l4_umword_t)(mgr_stack + MGR_STACKSIZE),
		    &pr, &pa, &dummy, &dummy, &dummy);

  /* now start serving the subtasks */
  pager();
}

/**
 * Transfer task create right for task no p.proto.param to sender.
 */

long
rmgr_get_task_component(CORBA_Object _dice_corba_obj,
                        long num,
                        CORBA_Server_Environment *_dice_corba_env)
{
  l4_threadid_t n;

  if (do_log)
    printf("ROOT: task_get sender=" l4util_idfmt " for task-nr=%lx\n",
	   l4util_idstr(*_dice_corba_obj), num);

  if (num >= RMGR_TASK_MAX)
    return 1;

  if (!task_alloc(num, _dice_corba_obj->id.task))
    return 1;

  reset_pagefault(num);

  n          = *_dice_corba_obj;
  n.id.task  = num;

  n = l4_task_new(n, (unsigned)_dice_corba_obj->raw, 0, 0, L4_NIL_ID);

  if (l4_is_nil_id(n))
    {
      task_free(num, _dice_corba_obj->id.task);
      return 1; /* failed */
    }

  return 0;
}

long
rmgr_set_prio_component(CORBA_Object _dice_corba_obj,
                        const l4_threadid_t *tid, long prio,
                        CORBA_Server_Environment *_dice_corba_env)
{
  l4_threadid_t s;
  l4_sched_param_t sched;

  if (do_log)
    printf("ROOT: task_set_prio sender=" l4util_idfmt
	   " for task=" l4util_idfmt " to prio=%lx\n",
           l4util_idstr(*_dice_corba_obj), l4util_idstr(*tid), prio);

  s = L4_INVALID_ID;
  l4_thread_schedule(*tid, L4_INVALID_SCHED_PARAM, &s, &s, &sched);
  if (l4_is_invalid_sched_param(sched)) /* error? */
    return 1;

  if (quota_get_log_mcp(_dice_corba_obj->id.task) < prio)
    return 1;

  sched.sp.prio = prio;
  s             = L4_INVALID_ID;
  l4_thread_schedule(*tid, sched, &s, &s, &sched);

  return 0;
}

long
rmgr_set_small_space_component(CORBA_Object _dice_corba_obj,
                               const l4_threadid_t *tid,
                               long num,
                               CORBA_Server_Environment *_dice_corba_env)
{
  l4_threadid_t s;
  l4_sched_param_t sched;

  if (do_log)
    printf("ROOT: set_small_space sender=" l4util_idfmt
	   " for task=" l4util_idfmt "\n",
           l4util_idstr(*_dice_corba_obj), l4util_idstr(*tid));

  s = L4_INVALID_ID;
  l4_thread_schedule(*tid, L4_INVALID_SCHED_PARAM, &s, &s, &sched);
  if (l4_is_invalid_sched_param(sched)) /* error? */
    return 1;

  if (!small_space_size
      || num >= 128 / small_space_size
      || !small_alloc(num, _dice_corba_obj->id.task))
    return 1;

  s              = L4_INVALID_ID;
  sched.sp.small = small_space_size | (num * small_space_size * 2);
  l4_thread_schedule(*tid, sched, &s, &s, &sched);

  return 0;
}

long
rmgr_task_new_component(CORBA_Object _dice_corba_obj,
                        const l4_taskid_t *tid,
                        l4_umword_t mcp_or_chief,
                        l4_umword_t sp,
                        l4_umword_t ip,
                        const l4_threadid_t *pager,
                        const l4_threadid_t *caphandler,
                        const l4_quota_desc_t *kquota,
                        l4_umword_t sched_param,
                        l4_taskid_t *ntid,
                        CORBA_Server_Environment *_dice_corba_env)
{
  l4_threadid_t n;
  l4_umword_t mcp;
  l4_umword_t flags;
  int task;

  if (do_log)
    printf("ROOT: task_create sender=" l4util_idfmt "\n",
           l4util_idstr(*_dice_corba_obj));

  n          = *tid;
  task       = n.id.task;

  if (task >= RMGR_TASK_MAX
      || l4_is_nil_id(*pager)
      || !task_alloc(task, _dice_corba_obj->id.task))
    return 1;

  flags = mcp_or_chief & L4_TASK_NEW_FLAGS_MASK;
  mcp_or_chief &= ~L4_TASK_NEW_FLAGS_MASK;

  mcp = mcp_or_chief > quota_get_log_mcp(_dice_corba_obj->id.task)
        ? quota_get_log_mcp(_dice_corba_obj->id.task) : mcp_or_chief;

  if (do_log)
    printf("ROOT: task_create task=" l4util_idfmt " sp=%p "
           "ip=%p pager=" l4util_idfmt " mcp=%ld\n",
           l4util_idstr(n), (void *)sp, (void *)ip,
           l4util_idstr(*pager), mcp);

  n = l4_task_new_long(n, mcp | flags, sp, ip, *pager, *caphandler, *kquota,
                       l4_utcb_get());

  if (l4_is_nil_id(n))
    {
      task_free(task, _dice_corba_obj->id.task);
      return 1;
    }

  /* for now, just reset all quotas to their minimal value */
  quota_reset(task);

  *ntid = n;

  return 0;
}

long
rmgr_free_task_all_component(CORBA_Object _dice_corba_obj,
                             const l4_threadid_t *tid,
                             CORBA_Server_Environment *_dice_corba_env)
{
  int i;

  if (do_log)
    printf("ROOT: task_free_all sender=" l4util_idfmt
	   " for task=" l4util_idfmt "\n",
           l4util_idstr(*_dice_corba_obj), l4util_idstr(*tid));

  for (i = 0; i < RMGR_TASK_MAX; i++)
    {
      if (task_owner(i) == tid->id.task)
	{
	  l4_threadid_t n;

	  reset_pagefault(i);

	  n          = *_dice_corba_obj;
	  n.id.task  = i;

	  n = l4_task_new(n, (unsigned)myself.raw, 0, 0, L4_NIL_ID);
	  if (l4_is_nil_id(n))
	    return 1;

	  task_free(i, tid->id.task);
	}
    }

  return 0;
}

long
rmgr_free_task_component(CORBA_Object _dice_corba_obj,
                         long num,
                         CORBA_Server_Environment *_dice_corba_env)
{

  if (do_log)
    printf("ROOT: task_free sender=" l4util_idfmt " for task-nr=%lx\n",
           l4util_idstr(*_dice_corba_obj), num);

  if (num >= RMGR_TASK_MAX)
    return 1;

  if (!task_free(num, _dice_corba_obj->id.task))
    return 1;

  return 0;
}

long
rmgr_get_task_id_component(CORBA_Object _dice_corba_obj,
                           const char* modname,
                           l4_threadid_t *tid,
                           CORBA_Server_Environment *_dice_corba_env)
{
  l4_threadid_t n = names_get_id(modname);

  if (do_log)
    printf("ROOT: task_get_id sender=" l4util_idfmt " for task-name=%s\n",
	   l4util_idstr(*_dice_corba_obj), modname);

  if (l4_is_invalid_id(n))
    return 1;

  *tid = n;

  return 0;
}

/**
 * the rmgr gets the id of a started task by another loader
 * case 1. task was rmgr boot module and with quota
 * case 2. task was NOT rmgr boot module but with with quota
 */
long
rmgr_set_task_id_component(CORBA_Object _dice_corba_obj,
                           const char* modname,
                           const l4_threadid_t *tid,
                           CORBA_Server_Environment *_dice_corba_env)
{
  l4_threadid_t id = *tid;

  id.id.lthread = 0;

  if (do_log)
    printf("ROOT: task_set_id sender=" l4util_idfmt
	   " for task=" l4util_idfmt " set task-name=%s\n",
           l4util_idstr(*_dice_corba_obj),
	   l4util_idstr(*tid), modname);

  if (task_owner(tid->id.task) == _dice_corba_obj->id.task)
    return cfg_quota_copy(id.id.task, modname);

  return -1;
}

long
rmgr_get_irq_component(CORBA_Object _dice_corba_obj,
                       int num,
                       CORBA_Server_Environment *_dice_corba_env)
{
  if (do_log)
    printf("ROOT: get_irq sender=" l4util_idfmt " irq=%x\n",
	l4util_idstr(*_dice_corba_obj), num);

  if (num >= RMGR_IRQ_MAX)
    return 1;

  if (irq_alloc(num, _dice_corba_obj->id.task))
    {
      /* the IRQ was free -- detach from it */
      l4_threadid_t n;
      l4_umword_t d1, d2;
      l4_msgdope_t result;
      int ret;

      n            = myself;
      n.id.lthread = LTHREAD_NO_IRQ(num);

      ret = l4_ipc_call(n, L4_IPC_SHORT_MSG, 1, 0,
                        L4_IPC_SHORT_MSG, &d1, &d2,
                        L4_IPC_NEVER, &result);

      if (!d1)
	return 0; /* success */
      
      /* failed */
      irq_free(num, _dice_corba_obj->id.task);
    }

  return 1;
}

long
rmgr_free_irq_component(CORBA_Object _dice_corba_obj,
                        int num,
                        CORBA_Server_Environment *_dice_corba_env)
{
  if (do_log)
    printf("ROOT: free_irq sender=" l4util_idfmt " irq=%x\n",
	l4util_idstr(*_dice_corba_obj), num);

  if (num < RMGR_IRQ_MAX
      && irq_free(num, _dice_corba_obj->id.task))
    {
      l4_umword_t d1, d2;
      l4_msgdope_t result;
      l4_threadid_t n;
      int ret;

      n            = myself;
      n.id.lthread = LTHREAD_NO_IRQ(num);

      ret = l4_ipc_call(n, L4_IPC_SHORT_MSG, 0, 0,
                        L4_IPC_SHORT_MSG, &d1, &d2,
                        L4_IPC_NEVER, &result);

      if (ret)
	{
	  printf("ROOT: free_irq: IPC error %d\n", ret);
	  return 1;
	}

      if (d1)
	return 1; /* failure */
    }

  return 0;
}

long
rmgr_free_irq_all_component(CORBA_Object _dice_corba_obj,
                            const l4_threadid_t *tid,
                            CORBA_Server_Environment *_dice_corba_env)
{
  int i;

  if (do_log)
    printf("ROOT: free_irq_all sender=" l4util_idfmt
	   " for task=" l4util_idfmt "\n",
	l4util_idstr(*_dice_corba_obj),
	l4util_idstr(*tid));

  for (i = 0; i < RMGR_IRQ_MAX; i++)
    {
      if ((irq_owner(i) == tid->id.task) && irq_free(i, tid->id.task))
	{
          l4_threadid_t n;
          l4_msgdope_t result;
	  l4_umword_t d1, d2;
	  int ret;

	  n            = myself;
	  n.id.lthread = LTHREAD_NO_IRQ(i);
	  ret = l4_ipc_call(n, L4_IPC_SHORT_MSG, 0, 0,
                            L4_IPC_SHORT_MSG, &d1, &d2,
                            L4_IPC_NEVER, &result);
	  if (ret)
	    printf("ROOT: free_irq_all: IPC error %d for irq %d", ret, i);
	}
    }

  return 0;
}

long
rmgr_dump_mem_component(CORBA_Object _dice_corba_obj,
                        CORBA_Server_Environment *_dice_corba_env)
{
  enter_memmap_functions(RMGR_LTHREAD_SUPER, rmgr_pager_id);
  memmap_dump();
  leave_memmap_functions(RMGR_LTHREAD_SUPER, rmgr_pager_id);
  return 0;
}

long
rmgr_free_page_component(CORBA_Object _dice_corba_obj,
                         l4_addr_t address,
                         CORBA_Server_Environment *_dice_corba_env)
{
  long ret = 1;

  if (do_log)
    printf("ROOT: free_page sender=" l4util_idfmt " for address=%lx\n",
	l4util_idstr(*_dice_corba_obj), address);

  enter_memmap_functions(RMGR_LTHREAD_SUPER, rmgr_pager_id);
  if (address >= 0x40000000 && address < 0xC0000000)
    {
      address += 0x40000000;

      if (_dice_corba_obj->id.task != memmap_owner_page(address)
          || !memmap_free_superpage(address, _dice_corba_obj->id.task))
	goto error;

      /* we can't unmap page here because the superpage was
       * granted so sender has to unmap page itself */
    }
  else
    {
      if (_dice_corba_obj->id.task != memmap_owner_page(address)
          || !memmap_free_page(address, _dice_corba_obj->id.task))
	goto error;
      else
	{
	  /* unmap fpage */
	  l4_fpage_unmap(l4_fpage(address, L4_LOG2_PAGESIZE,
                                  L4_FPAGE_RW, L4_FPAGE_MAP),
                         L4_FP_FLUSH_PAGE | L4_FP_OTHER_SPACES);
	}
    }

  ret = 0;

error:
  leave_memmap_functions(RMGR_LTHREAD_SUPER, rmgr_pager_id);
  return ret;
}

long
rmgr_free_fpage_component(CORBA_Object _dice_corba_obj,
                          l4_umword_t fp,
                          CORBA_Server_Environment *_dice_corba_env)
{
  if (do_log)
    printf("ROOT: free_fpage sender=" l4util_idfmt "\n",
	l4util_idstr(*_dice_corba_obj));

#ifdef ARCH_x86
  if (l4_is_io_page_fault(fp))
    {
      unsigned i;
      unsigned port = ((l4_fpage_t)fp).iofp.iopage;
      unsigned size = ((l4_fpage_t)fp).iofp.iosize;

  if (do_log)
    printf("ROOT: free_fpage for port=%x and size=%x\n", port, size);

      /* don't worry about errors */
      for (i = port; i < port + (1 << size); i++)
	iomap_free_port(i, _dice_corba_obj->id.task);

      return 0;
    }
#endif

  if (do_log)
    printf("ROOT: free_fpage Unable to handle free fpage request.\n");

  return 1; /* failure */
}

long
rmgr_free_mem_all_component(CORBA_Object _dice_corba_obj,
                            const l4_threadid_t *tid,
                            CORBA_Server_Environment *_dice_corba_env)
{
  l4_addr_t p, pa, q, qa;
  /* scan all physical memory */

  if (do_log)
    printf("ROOT: free_mem_all sender=" l4util_idfmt
	   " for task=" l4util_idfmt "\n",
	l4util_idstr(*_dice_corba_obj),
	l4util_idstr(*tid));

  enter_memmap_functions(RMGR_LTHREAD_SUPER, rmgr_pager_id);
  for (p = 0; p < SUPERPAGE_MAX; p++)
    {
      pa = p << L4_SUPERPAGESHIFT;
      if (memmap_owner_superpage(pa) == tid->id.task
	/* superpage belongs to exactly the one owner */
	  && memmap_free_superpage(pa, tid->id.task))
	{
          l4_fpage_unmap(l4_fpage(pa, L4_LOG2_SUPERPAGESIZE,
                                  L4_FPAGE_RW, L4_FPAGE_MAP),
                         L4_FP_FLUSH_PAGE | L4_FP_OTHER_SPACES);
	}

      if (memmap_owner_superpage(pa) == O_RESERVED)
	{
	  /* superpage belongs to more than one owner */
	  for (q = 0; q < L4_SUPERPAGESIZE/L4_PAGESIZE; q++)
	    {
	      qa = pa + (q << L4_PAGESHIFT);
	      if (memmap_owner_page(qa) == tid->id.task
                  && memmap_free_page(qa, tid->id.task))
		{
#if 0
		  l4_fpage_unmap(l4_fpage(qa, L4_LOG2_PAGESIZE,
					  L4_FPAGE_RW, L4_FPAGE_MAP),
				 L4_FP_FLUSH_PAGE | L4_FP_OTHER_SPACES);
#endif
		}
	    }
	}
    }


  /* scan adapter space */
  for (p = 0x40000000; p < 0xC0000000; p += L4_SUPERPAGESIZE)
    memmap_free_superpage(p, tid->id.task);

  leave_memmap_functions(RMGR_LTHREAD_SUPER, rmgr_pager_id);
  return 0; /* success */
}

long
rmgr_reserve_mem_component(CORBA_Object _dice_corba_obj,
                           l4_addr_t size, l4_addr_t align, int flags,
                           l4_addr_t low, l4_addr_t high, l4_addr_t *addr,
                           CORBA_Server_Environment *_dice_corba_env)
{
  l4_addr_t a;

  if (do_log)
    printf("ROOT: reserve_mem sender=" l4util_idfmt "\n",
	l4util_idstr(*_dice_corba_obj));

  enter_memmap_functions(RMGR_LTHREAD_SUPER, rmgr_pager_id);

  size = l4_round_page(size);
  if (align < L4_LOG2_PAGESIZE)
    align = L4_LOG2_PAGESIZE;
  else if (align > 31)
    align = 31;
  flags &= RMGR_MEM_RES_FLAGS_MASK;

  a = reserve_range(size | align | flags, _dice_corba_obj->id.task, low, high);
  if (do_log)
    printf("ROOT: reserve_mem: addr:%lx\n", a);

  if (a != -1)
    *addr = a;

  leave_memmap_functions(RMGR_LTHREAD_SUPER, rmgr_pager_id);

  return a != -1;
}

void
rmgr_get_page0_component(CORBA_Object _dice_corba_obj,
			 l4_snd_fpage_t *page0,
                         CORBA_Server_Environment *_dice_corba_env)
{
  if (do_log)
    printf("ROOT: get_page0 sender=" l4util_idfmt "\n",
	l4util_idstr(*_dice_corba_obj));

  page0->snd_base = 0;
  page0->fpage = l4_fpage(0, L4_LOG2_PAGESIZE, L4_FPAGE_RW, L4_FPAGE_MAP);
}

void
rmgr_init_ping_component(CORBA_Object _dice_corba_obj,
                         long value, long *not_value,
                         CORBA_Server_Environment *_dice_corba_env)
{
  if (do_log)
    printf("ROOT: init_ping sender=" l4util_idfmt " for value=%lx\n",
	l4util_idstr(*_dice_corba_obj), value);

  *not_value = ~value;
}

long
rmgr_get_prio_component(CORBA_Object _dice_corba_obj,
                        const l4_threadid_t *tid, long *prio,
                        CORBA_Server_Environment *_dice_corba_env)
{
  printf("ROOT: get_prio Implemented on client side.\n");
  return 1;
}

long
rmgr_privctrl_component(CORBA_Object _dice_corba_obj,
			int command, int param,
			CORBA_Server_Environment *_dice_corba_env)
{
  return l4_privctrl(1, _dice_corba_obj->id.task);
}

static void
mgr(void)
{
  rmgr_server_loop(NULL);
}
