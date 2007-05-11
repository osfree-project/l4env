/**
 * \file   omega0/server/src/irq_threads.h
 * \brief  IRQ threads API
 *
 * \date   2007-04-27
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __OMEGA0_SERVER_IRQ_THREADS_H
#define __OMEGA0_SERVER_IRQ_THREADS_H

#include "globals.h"

extern int attach_irqs(void);
extern void check_auto_consume(int irq, client_chain *c);

void irq_handler(int num);

#endif
