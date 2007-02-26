#ifndef __OMEGA0_SERVER_PIC_H
#define __OMEGA0_SERVER_PIC_H

void irq_mask(int);
void irq_unmask(int);
void irq_ack(int irq);
int pic_isr(int master);
int pic_irr(int master);
int pic_imr(int master);
#endif
