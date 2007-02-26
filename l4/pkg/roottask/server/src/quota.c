/**
 * \file	roottask/server/src/quota.c
 * \brief	quota implementation for tasks
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/
#include <stdio.h>
#include <string.h>
#include "quota.h"
#include "memmap.h"
#include "rmgr.h"
#include "misc.h"
#include <l4/sys/kdebug.h>

/* the task quota is filled after starting a
 * 1. for boot tasks after booting
 * 2. for other tasks via  RMGR_SET_ID service
 **/
static quota_t quota[RMGR_TASK_MAX];

/* the configuration quota is an associative buffer and
 * is filled while parsing the config file
 * for tasks we dont know the exact taskid
 *
 * later if RMGR knows the taskid it copies the quotas
 * to correct place
 **/

typedef struct
{
  char       name[MOD_NAME_MAX];
  quota_t    quota;
} cfg_quota_t;

static cfg_quota_t cfg_quota[RMGR_CFG_MAX];
static unsigned    max_cfg_quota;

static inline int
__quota_check_addr(addr_quota_t q, l4_addr_t where, unsigned amount)
{
  return ( (q.max >= amount && q.current <= q.max - amount) ?
              ((q.low <= where && q.high >= where + amount - 1) ?
	       1 : 0) : 0);
}

static inline int
__quota_check_u16(u16_quota_t q, l4_addr_t where, unsigned amount)
{
  return ( (q.max >= amount && q.current <= q.max - amount) ?
              ((q.low <= where && q.high >= where + amount - 1) ?
	       1 : 0) : 0);
}

static inline int
__quota_check_u8(u8_quota_t q, l4_addr_t where, unsigned amount)
{
  return ( (q.max >= amount && q.current <= q.max - amount) ?
              ((q.low <= where && q.high >= where + amount - 1) ?
	       1 : 0) : 0);
}

static inline int
__quota_free_addr(addr_quota_t q, unsigned amount)
{
   return (q.current -= amount);
}

static inline int
__quota_free_u16(u16_quota_t q, unsigned amount)
{
   return (q.current -= amount);
}

static inline int
__quota_free_u8(u8_quota_t q, unsigned amount)
{
   return (q.current -= amount);
}

void
quota_init(void)
{
  unsigned i;

  for (i = 0; i < RMGR_TASK_MAX; i++)
    {
      quota[i].mem.low    = quota[i].mem.current   =  0;
      quota[i].mem.high   = quota[i].mem.max       = -1;
      quota[i].himem.low  = quota[i].himem.current =  0;
      quota[i].himem.high = quota[i].himem.max     = -1;
      quota[i].task.low   = quota[i].task.current  =  0;
      quota[i].task.high  = quota[i].task.max      = -1;
      quota[i].small.low  = quota[i].small.current =  0;
      quota[i].small.high = quota[i].small.max     = -1;
      quota[i].irq                                 = (1 << RMGR_IRQ_MAX)-1;
      quota[i].allow_cli                           =  0;
      quota[i].log_mcp                             = -1;
    }
}

void
quota_init_log_mcp(int log_mcp)
{
  unsigned i;
  for (i = 0; i < RMGR_TASK_MAX; i++)
    quota[i].log_mcp = log_mcp;
}

void
quota_init_mcp(int mcp)
{
#if 0
  unsigned i;
  for (i = 0; i < RMGR_TASK_MAX; i++)
    bootquota[i].mcp = mcp;
#endif
  enter_kdebug("quota_init_mcp not implemented");
}

void
quota_init_prio(int prio)
{
#if 0
  unsigned i;
  for (i = 0; i < RMGR_TASK_MAX; i++)
    bootquota[i].prio = prio;
#endif
  enter_kdebug("quota_init_prio not implemented");
}

void
quota_set(unsigned i, const quota_t * const q)
{
  quota[i] = *q;
}


const char*
cfg_quota_set(const char *cfg_name, const quota_t * const q)
{
  if (max_cfg_quota < RMGR_CFG_MAX)
    {
      snprintf(cfg_quota[max_cfg_quota].name, sizeof(cfg_quota[0].name),
	       "%s", cfg_name);
      cfg_quota[max_cfg_quota].quota = *q;
      return cfg_quota[max_cfg_quota++].name;
    }

  return 0;
}

void
cfg_quota_copy(unsigned i, const char *name)
{
  int j;

  for (j = 0; j < max_cfg_quota; j++)
    if (is_program_in_cmdline(name, cfg_quota[j].name))
      {
	quota[i] = cfg_quota[j].quota;
	return;
      }
}

void
quota_reset(owner_t task)
{
  quota[task].mem.max   = 0;
  quota[task].himem.max = 0;
  quota[task].task.max  = 0;
  quota[task].small.max = 0;
  quota[task].irq       = (1 << RMGR_IRQ_MAX) -1; /* XXX: hack */
  quota[task].allow_cli = 0;
  quota[task].log_mcp   = 0; /* mcp */
}

int
quota_check_mem(owner_t t, l4_addr_t where, unsigned amount)
{
  return (where < mem_high)
	    ? __quota_check_addr(quota[t].mem, where, amount)
    	    : __quota_check_addr(quota[t].himem, where, amount);
}

int
quota_check_allow_cli(owner_t t)
{
  return quota[t].allow_cli;
}

int
quota_alloc_mem(owner_t t, l4_addr_t where, unsigned amount)
{
  int res;

  if (where < mem_high)
    {
      if ((res = __quota_check_addr(quota[t].mem, where,amount)))
        quota[t].mem.current += amount;
    }
  else
    {
      if ((res = __quota_check_addr(quota[t].himem, where,amount)))
        quota[t].himem.current += amount;
    }

  return res;
}

int
quota_alloc_task(owner_t t, l4_addr_t where)
{
  int res;

  if ((res = __quota_check_u16(quota[t].task, (where), 1)))
    quota[t].task.current += 1;

  return res;
}

int
quota_alloc_small(owner_t t, l4_addr_t where)
{
  int res;

  if ((res = __quota_check_u8(quota[t].small, (where), 1)))
    quota[t].small.current += 1;

  return res;
}

int
quota_alloc_irq(owner_t t, l4_uint32_t where)
{
  return ((quota[t].irq & (1L << where)) ? 1 : 0);
}

int
quota_free_mem(owner_t t, l4_addr_t where, unsigned amount)
{
  return (where < mem_high)
	    ? __quota_free_addr(quota[t].mem, amount)
	    : __quota_free_addr(quota[t].himem, amount);
}

int
quota_free_task(owner_t t, l4_addr_t where)
{
  return __quota_free_u16(quota[t].task, 1);
}

int
quota_free_small(owner_t t, l4_addr_t where)
{
  return __quota_free_u8(quota[t].small, 1);
}

int
quota_free_irq(owner_t t, l4_addr_t where)
{
  /* nothing to free here */
  return 0;
}

void
quota_add_mem(owner_t t, unsigned amount)
{
  quota[t].mem.current += amount;
}

quota_t*
quota_get(unsigned n)
{
  return &quota[n];
}

int
quota_get_log_mcp(owner_t t)
{
  return quota[t].log_mcp;
}

inline unsigned
quota_get_offset(owner_t t)
{
  return quota[t].offset;
}

void
quota_dump(owner_t t)
{
  printf("task.curr: %d  ", quota[t].task.current);
  printf("task.low:  %d  ", quota[t].task.low);
  printf("task.high: %d  ", quota[t].task.high);
  printf("task.max:  %d\n", quota[t].task.max);
}

void
quota_set_default(quota_t *q)
{
  q->mem.low    = q->mem.current   =  0;
  q->mem.high   = q->mem.max       = -1;
  q->himem.low  = q->himem.current =  0;
  q->himem.high = q->himem.max     = -1;
  q->task.low   = q->task.current  =  0;
  q->task.high  = q->task.max      = -1;
  q->small.low  = q->small.current =  0;
  q->small.high = q->small.max     = -1;
  q->irq        = (1 << RMGR_IRQ_MAX)-1;
  q->allow_cli  =  0;
  q->log_mcp    = -1;
  q->offset     = 0;
}

int
quota_is_default_mem(quota_t *q)
{
  return q->mem.low   == 0 &&
         q->mem.high  == (l4_addr_t)-1 &&
	 q->mem.max   == (l4_addr_t)-1;
}

int
quota_is_default_himem(quota_t *q)
{
  return q->himem.low  == 0 &&
         q->himem.high == (l4_addr_t)-1 &&
	 q->himem.max  == (l4_addr_t)-1;
}

int
quota_is_default_task(quota_t *q)
{
  return q->task.low  == 0 &&
	 q->task.high == (l4_uint16_t)-1 &&
	 q->task.max  == (l4_uint16_t)-1;
}

int
quota_is_default_small(quota_t *q)
{
  return q->small.low  == 0  &&
         q->small.high == (l4_uint8_t)-1 &&
	 q->small.max  == (l4_uint8_t)-1;
}

int
quota_is_default_misc(quota_t *q)
{
  return q->irq       == (1 << RMGR_IRQ_MAX)-1 &&
         q->allow_cli == 0 &&
	 q->log_mcp   == (l4_uint16_t)-1 &&
	 q->offset    == 0;
}
