/*!
 * \file   omega0/examples/delay/serial.h
 * \brief  Serial code prototypes
 *
 * \date   01/29/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */

#ifndef __OMEGA0_EXAMPLES_DELAY_SERIAL_H_
#define __OMEGA0_EXAMPLES_DELAY_SERIAL_H_
#include <l4/sys/compiler.h>

extern void serial_init(int,int);

/* init the serial line belonging to port with 115000 baud, 8n1,
   irq's on incoming and delivery. No pic programming will take place. */
extern void serial_init(int port, int irq);
extern void serial_send_char(int port, char snd);

#endif
