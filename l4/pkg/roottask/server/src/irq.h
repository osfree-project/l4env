/**
 * \file	roottask/server/src/irq.h
 * \brief	IRQ resource handling
 *
 * \date	05/10/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author      Torsten Frenzel <frenzel@os.inf.tu-dresden.de>
 *
 **/
#ifndef IRQ_H
#define IRQ_H

#include "types.h"

void irq_init(void);
int  irq_alloc(unsigned irqno, owner_t owner);
int  irq_free(unsigned irqno, owner_t owner);
int  irq_owner(unsigned irqno);

#endif
