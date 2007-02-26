/*!
 * \file   omega0/examples/delay/pic.h
 * \brief  PIC handling prototypes
 *
 * \date   01/29/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */

#ifndef __OMEGA0_EXAMPLES_DELAY_PIC_H_
#define __OMEGA0_EXAMPLES_DELAY_PIC_H_

extern void irq_mask(int irq);
extern void irq_unmask(int irq);
extern void irq_ack(int irq);
extern int pic_isr(int master);
extern int pic_irr(int master);
extern int pic_imr(int master);

#endif
