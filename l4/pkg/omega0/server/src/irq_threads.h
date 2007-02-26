#ifndef __OMEGA0_SERVER_IRQ_THREADS_H
#define __OMEGA0_SERVER_IRQ_THREADS_H

#include "globals.h"

extern int attach_irqs(void);
extern void check_auto_consume(int irq, client_chain *c);

/* krishna: exported for separate thread creation */
void irq_handler(int num);

extern int use_special_fully_nested;

#endif
