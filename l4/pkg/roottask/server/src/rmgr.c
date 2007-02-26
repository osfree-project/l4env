#include <stdio.h>
#include <unistd.h>

#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/util/l4_macros.h>
#include <l4/rmgr/librmgr.h>
#include <l4/rmgr/proto.h>

#ifndef USE_OSKIT
#include <l4/env_support/panic.h>
#include <l4/env_support/getchar.h>
#endif

#include "exec.h"
#include "globals.h"
#include "memmap.h"
#include "memmap_lock.h"
#include "names.h"
#include "rmgr.h"
#include "pager.h"
#include "irq.h"
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

static char mgr_stack[MGR_STACKSIZE] __attribute__((aligned(4)));
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

  /* free .init section */
  for (address  = l4_trunc_page(&_start);
       address  < l4_trunc_page(&_stext);
       address += L4_PAGESIZE)
    {
      memset((void*)address, 0, L4_PAGESIZE);
      if (!memmap_free_page(address, O_RESERVED))
	printf ("Error freeing .init at %08x", address);
    }
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

l4_int32_t
rmgr_get_task_component(CORBA_Object _dice_corba_obj,
                        l4_int32_t num,
                        CORBA_Server_Environment *_dice_corba_env)
{
  l4_threadid_t n;

  if (do_log)
    printf("RMGR: task_get sender=" l4util_idfmt " for task-nr=%x\n",
	   l4util_idstr(*_dice_corba_obj), num);

  if (num >= RMGR_TASK_MAX)
    return 1;

  if (!task_alloc(num, _dice_corba_obj->id.task))
    return 1;

  reset_pagefault(num);

  n          = *_dice_corba_obj;
  n.id.task  = num;
  n.id.chief = l4_myself().id.task;

  n = l4_task_new(n, (unsigned)_dice_corba_obj->raw, 0, 0, L4_NIL_ID);

  if (l4_is_nil_id(n))
    {
      task_free(num, _dice_corba_obj->id.task);
      return 1; /* failed */
    }

  return 0;
}

l4_int32_t
rmgr_set_prio_component(CORBA_Object _dice_corba_obj,
                        const l4_threadid_t *tid, l4_int32_t prio,
                        CORBA_Server_Environment *_dice_corba_env)
{
  l4_threadid_t s;
  l4_sched_param_t sched;

  if (do_log)
    printf("RMGR: task_set_prio sender=" l4util_idfmt
	   " for task=" l4util_idfmt " to prio=%x\n",
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

l4_int32_t
rmgr_set_small_space_component(CORBA_Object _dice_corba_obj,
                               const l4_threadid_t *tid,
                               l4_int32_t num,
                               CORBA_Server_Environment *_dice_corba_env)
{
  l4_threadid_t s;
  l4_sched_param_t sched;

  if (do_log)
    printf("RMGR: set_small_space sender=" l4util_idfmt
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

l4_int32_t
rmgr_task_new_component(CORBA_Object _dice_corba_obj,
                        const l4_taskid_t *tid,
                        l4_umword_t mcp_or_chief,
                        l4_umword_t esp,
                        l4_umword_t eip,
                        const l4_threadid_t *pager,
                        l4_umword_t sched_param,
                        l4_taskid_t *ntid,
                        CORBA_Server_Environment *_dice_corba_env)
{
  l4_threadid_t n;
  l4_umword_t mcp;
  l4_umword_t alien;
  int task;

  if (do_log)
    printf("RMGR: task_create sender=" l4util_idfmt "\n",
           l4util_idstr(*_dice_corba_obj));

  n          = *tid;
  n.id.chief = myself.id.task;
  task       = n.id.task;

  if (task >= RMGR_TASK_MAX
      || l4_is_nil_id(*pager)
      || !task_alloc(task, _dice_corba_obj->id.task))
    return 1;

  alien = mcp_or_chief & L4_TASK_NEW_ALIEN;
  mcp_or_chief &= ~L4_TASK_NEW_ALIEN;

  mcp = mcp_or_chief > quota_get_log_mcp(_dice_corba_obj->id.task)
        ? quota_get_log_mcp(_dice_corba_obj->id.task) : mcp_or_chief;

  if (do_log)
    printf("RMGR: task_create task=" l4util_idfmt " esp=%p "
           "eip=%p pager=" l4util_idfmt " mcp=%d\n",
           l4util_idstr(n), (void *)esp, (void *)eip,
           l4util_idstr(*pager), mcp);

  n = l4_task_new(n, mcp | alien, esp, eip, *pager);

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

l4_int32_t
rmgr_free_task_all_component(CORBA_Object _dice_corba_obj,
                             const l4_threadid_t *tid,
                             CORBA_Server_Environment *_dice_corba_env)
{
  int i;

  if (do_log)
    printf("RMGR: task_free_all sender=" l4util_idfmt
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
	  n.id.chief = myself.id.task;

	  n = l4_task_new(n, (unsigned)myself.raw, 0, 0, L4_NIL_ID);
	  if (l4_is_nil_id(n))
	    return 1;

	  task_free(i, tid->id.task);
	}
    }

  return 0;
}

l4_int32_t
rmgr_free_task_component(CORBA_Object _dice_corba_obj,
                         l4_int32_t num,
                         CORBA_Server_Environment *_dice_corba_env)
{

  if (do_log)
    printf("RMGR: task_free sender=" l4util_idfmt " for task-nr=%x\n",
           l4util_idstr(*_dice_corba_obj), num);

  if (num >= RMGR_TASK_MAX)
    return 1;

  if (!task_free(num, _dice_corba_obj->id.task))
    return 1;

  return 0;
}

l4_int32_t
rmgr_get_task_id_component(CORBA_Object _dice_corba_obj,
                           const char* modname,
                           l4_threadid_t *tid,
                           CORBA_Server_Environment *_dice_corba_env)
{
  l4_threadid_t n = names_get_id(modname);

  if (do_log)
    printf("RMGR: task_get_id sender=" l4util_idfmt " for task-name=%s\n",
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
l4_int32_t
rmgr_set_task_id_component(CORBA_Object _dice_corba_obj,
                           const char* modname,
                           const l4_threadid_t *tid,
                           CORBA_Server_Environment *_dice_corba_env)
{
  l4_threadid_t id = *tid;

  id.id.lthread = 0;

  if (do_log)
    printf("RMGR: task_set_id sender=" l4util_idfmt
	   " for task=" l4util_idfmt " set task-name=%s\n",
           l4util_idstr(*_dice_corba_obj),
	   l4util_idstr(*tid), modname);

  if (task_owner(tid->id.task) == _dice_corba_obj->id.task)
    {
      names_set(id, modname);
      cfg_quota_copy(id.id.task, modname);
    }

  return 0;
}

l4_int32_t
rmgr_get_irq_component(CORBA_Object _dice_corba_obj,
                       l4_int32_t num,
                       CORBA_Server_Environment *_dice_corba_env)
{
  if (do_log)
    printf("RMGR: get_irq sender=" l4util_idfmt " irq=%x\n",
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

l4_int32_t
rmgr_free_irq_component(CORBA_Object _dice_corba_obj,
                        l4_int32_t num,
                        CORBA_Server_Environment *_dice_corba_env)
{
  if (do_log)
    printf("RMGR: free_irq sender=" l4util_idfmt " irq=%x\n",
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
	  printf("RMGR: free_irq: IPC error %d\n", ret);
	  return 1;
	}

      if (d1)
	return 1; /* failure */
    }

  return 0;
}

l4_int32_t
rmgr_free_irq_all_component(CORBA_Object _dice_corba_obj,
                            const l4_threadid_t *tid,
                            CORBA_Server_Environment *_dice_corba_env)
{
  int i;

  if (do_log)
    printf("RMGR: free_irq_all sender=" l4util_idfmt
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
	    printf("RMGR: free_irq_all: IPC error %d for irq %d", ret, i);
	}
    }

  return 0;
}

l4_int32_t
rmgr_dump_mem_component(CORBA_Object _dice_corba_obj,
                        CORBA_Server_Environment *_dice_corba_env)
{
  printf("RMGR: dump_mem NOT implemented (called by "l4util_idfmt")\n",
      l4util_idstr(*_dice_corba_obj));
  return 0;
}

l4_int32_t
rmgr_free_page_component(CORBA_Object _dice_corba_obj,
                         l4_addr_t address,
                         CORBA_Server_Environment *_dice_corba_env)
{
  if (do_log)
    printf("RMGR: free_page sender=" l4util_idfmt " for address=%x\n",
	l4util_idstr(*_dice_corba_obj), address);

  if (address >= 0x40000000 && address < 0xC0000000)
    {
      address += 0x40000000;

      if (_dice_corba_obj->id.task != memmap_owner_page(address)
          || !memmap_free_superpage(address, _dice_corba_obj->id.task))
	return 1; /* failure */

      /* we can't unmap page here because the superpage was
       * granted so sender has to unmap page itself */
    }
  else
    {
      if (_dice_corba_obj->id.task != memmap_owner_page(address)
          || !memmap_free_page(address, _dice_corba_obj->id.task))
	return 1; /* failure */
      else
	{
	  /* unmap fpage */
	  l4_fpage_unmap(l4_fpage(address, L4_LOG2_PAGESIZE,
                                  L4_FPAGE_RW, L4_FPAGE_MAP),
                         L4_FP_FLUSH_PAGE | L4_FP_OTHER_SPACES);
	}
    }

  return 0;
}

l4_int32_t
rmgr_free_fpage_component(CORBA_Object _dice_corba_obj,
                          l4_umword_t fp,
                          CORBA_Server_Environment *_dice_corba_env)
{
  if (do_log)
    printf("RMGR: free_fpage sender=" l4util_idfmt "\n",
	l4util_idstr(*_dice_corba_obj));

#ifdef ARCH_x86
  if (l4_is_io_page_fault(fp))
    {
      unsigned i;
      unsigned port = ((l4_fpage_t)fp).iofp.iopage;
      unsigned size = ((l4_fpage_t)fp).iofp.iosize;

  if (do_log)
    printf("RMGR: free_fpage for port=%x and size=%x\n", port, size);

      /* don't worry about errors */
      for (i = port; i < port + (1 << size); i++)
	iomap_free_port(i, _dice_corba_obj->id.task);

      return 0;
    }
#endif

  if (do_log)
    printf("RMGR: free_fpage Unable to handle free fpage request.\n");

  return 1; /* failure */
}

l4_int32_t
rmgr_free_mem_all_component(CORBA_Object _dice_corba_obj,
                            const l4_threadid_t *tid,
                            CORBA_Server_Environment *_dice_corba_env)
{
  unsigned long p;
  /* scan all physical memory */

  if (do_log)
    printf("RMGR: free_mem_all sender=" l4util_idfmt
	   " for task=" l4util_idfmt "\n",
	l4util_idstr(*_dice_corba_obj),
	l4util_idstr(*tid));

  for (p = 0; p < MEM_MAX; p+= L4_SUPERPAGESIZE)
    {
      if (memmap_owner_superpage(p) == tid->id.task
	/* superpage belongs to exactly the one owner */
	  && memmap_free_superpage(p, tid->id.task))
	{
          l4_fpage_unmap(l4_fpage(p, L4_LOG2_SUPERPAGESIZE,
                                  L4_FPAGE_RW, L4_FPAGE_MAP),
                         L4_FP_FLUSH_PAGE | L4_FP_OTHER_SPACES);
	}

      if (memmap_owner_superpage(p) == O_RESERVED)
	{
	  unsigned long q;

	  /* superpage belongs to more than one owner */
	  for (q = p; q < p + L4_SUPERPAGESIZE; q += L4_PAGESIZE)
	    {
	      if (memmap_owner_page(p) == tid->id.task
                  && memmap_free_page(q, tid->id.task))
		{
#if 0
		  l4_fpage_unmap(l4_fpage(q, L4_LOG2_PAGESIZE,
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

  return 0; /* success */
}

l4_int32_t
rmgr_reserve_mem_component(CORBA_Object _dice_corba_obj,
                           l4_addr_t size, l4_addr_t align, l4_int32_t flags,
                           l4_addr_t low, l4_addr_t high, l4_addr_t *addr,
                           CORBA_Server_Environment *_dice_corba_env)
{
  l4_addr_t a;

  if (do_log)
    printf("RMGR: reserve_mem sender=" l4util_idfmt "\n",
	l4util_idstr(*_dice_corba_obj));

  size = l4_round_page(size);
  if (align < L4_LOG2_PAGESIZE)
    align = L4_LOG2_PAGESIZE;
  else if (align > 31)
    align = 31;
  flags &= RMGR_MEM_RES_FLAGS_MASK;

  a = reserve_range(size | align | flags, _dice_corba_obj->id.task, low, high);
  if (a != -1)
    *addr = a;

  if (do_log)
    printf("RMGR: Log: addr:%x\n", *addr);

  return !!a;
}

l4_int32_t
rmgr_get_page0_component(CORBA_Object _dice_corba_obj,
			 l4_snd_fpage_t *page0,
                         CORBA_Server_Environment *_dice_corba_env)
{
  if (do_log)
    printf("RMGR: get_page0 sender=" l4util_idfmt "\n",
	l4util_idstr(*_dice_corba_obj));

  page0->snd_base = 0;
  page0->fpage = l4_fpage(0, L4_LOG2_PAGESIZE, L4_FPAGE_RW, L4_FPAGE_MAP);

  return 0;
}

l4_int32_t
rmgr_init_ping_component(CORBA_Object _dice_corba_obj,
                         l4_int32_t value, l4_int32_t *not_value,
                         CORBA_Server_Environment *_dice_corba_env)
{
  if (do_log)
    printf("RMGR: init_ping sender=" l4util_idfmt " for value=%x\n",
	l4util_idstr(*_dice_corba_obj), value);

  *not_value = ~value;
  return 0;
}

l4_int32_t
rmgr_get_prio_component(CORBA_Object _dice_corba_obj,
                        const l4_threadid_t *tid, l4_int32_t *prio,
                        CORBA_Server_Environment *_dice_corba_env)
{
  printf("RMGR: get_prio Implemented on client side.\n");
  return 1;
}

l4_int32_t
rmgr_privctrl_component(CORBA_Object _dice_corba_obj,
			l4_int32_t command, l4_int32_t param,
			CORBA_Server_Environment *_dice_corba_env)
{
  return l4_privctrl(1, _dice_corba_obj->id.task);
}

static void
mgr(void)
{
  rmgr_server_loop(NULL);
}
