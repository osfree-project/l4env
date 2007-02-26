/*!
 * \file   omega0/examples/pit/pic.h
 * \brief  PIC manipulation prototypes
 *
 * \date   09/15/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __OMEGA0_EXAMPLES_PIT_PIC_H_
#define __OMEGA0_EXAMPLES_PIT_PIC_H_
void irq_mask(int irq);
void irq_unmask(int irq);
void irq_ack(int irq);
int pic_isr(int master);
int pic_irr(int master);
int pic_imr(int master);

#endif
