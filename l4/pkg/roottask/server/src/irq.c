/**
 * \file	roottask/server/src/irq.c
 * \brief	IRQ resource handling
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/
#include <string.h>

#include "irq.h"
#include "quota.h"
#include "rmgr.h"

static owner_t __irq[RMGR_IRQ_MAX];


void irq_init(void)
{
  memset(__irq, O_RESERVED, RMGR_IRQ_MAX);
}

int
irq_alloc(unsigned irqno, owner_t owner)
{
  if (__irq[irqno] == owner)
    return 1;
  if (__irq[irqno] != O_FREE)
    return 0;

  if (! quota_alloc_irq(owner, irqno))
    return 0;

  __irq[irqno] = owner;
  return 1;
}

int
irq_free(unsigned irqno, owner_t owner)
{
  if (__irq[irqno] != owner && __irq[irqno] != O_FREE)
    return 0;

  if (__irq[irqno] != O_FREE)
    {
      quota_free_irq(owner, irqno);
      __irq[irqno] = O_FREE;
    }

  return 1;
}

int
irq_owner(unsigned irqno)
{
  return __irq[irqno];
}
