/*!
 * \file   serial/lib/src/serial_op.h
 * \brief  Serial line basics
 *
 * \date   10/21/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __SERIAL_LIB_SRC_SERIAL_OP_H_
#define __SERIAL_LIB_SRC_SERIAL_OP_H_
#include <l4/sys/compiler.h>
#include <l4/util/port_io.h>
#include <l4/sys/l4int.h>

#define SERIAL_THR 0	/* w:  THR Transmit holding register */
#define SERIAL_RHR 0	/* r:  RHR Receive holding register */
#define SERIAL_IER 1	/* rw: IER Interrupt enable register */
#define SERIAL_FCR 2	/* w:  FCR FIFO control register */
#define SERIAL_ISR 2	/* r:  ISR Interrupt status register */
#define SERIAL_LCR 3	/* rw: LCR Line control register */
#define SERIAL_MCR 4	/* rw: MCR Modem control register */
#define SERIAL_LSR 5	/* r:  LSR Line status register */
#define SERIAL_MSR 6	/* r:  MSR Modem status register */
#define SERIAL_SPR 7	/* rw: SPR Scratch pad register */
#define SERIAL_DLL 0	/* rw: DLL Divisor latch, low word.
			 *     to access, DLAB=LCR[7] must be 1 */
#define SERIAL_DLM 1	/* rw: DLM Divisor latch, high word.
			 *     to access, DLAB=LCR[7] must be 1 */
#define SERIAL_EFR 2	/* rw: EFR Enhanced feature register.
			 *     to access, DLAB=LCR[7] must be 1, and
			 *                LCR=0xBF. */

L4_INLINE void serial_setup(int port);
L4_INLINE void serial_sendchar(int port, char c);
L4_INLINE void serial_enable_bits(int port, int off, int mask);
L4_INLINE void serial_disable_bits(int port, int off, int mask);
L4_INLINE void serial_set(int port, int off, int val);
L4_INLINE l4_uint8_t serial_get(int port, int off);
L4_INLINE void serial_enable_dlab(int port);
L4_INLINE void serial_disable_dlab(int port);
L4_INLINE void serial_set_divisor(int port, l4_uint16_t divisor);
L4_INLINE l4_uint8_t serial_enable_EFR(int port);
L4_INLINE void serial_disable_EFR(int port, l4_uint8_t oldval);


/*******************************************************************
 * Implementation
 *******************************************************************/


L4_INLINE void serial_enable_bits(int port, int off, int mask){
    l4util_out8(l4util_in8(port+off) | mask, port+off);
}
L4_INLINE void serial_disable_bits(int port, int off, int mask){
    l4util_out8(l4util_in8(port+off) & ~mask, port+off);
}
L4_INLINE void serial_set(int port, int off, int val){
    l4util_out8(val, port+off);
}
L4_INLINE l4_uint8_t serial_get(int port, int off){
    return l4util_in8(port+off);
}

L4_INLINE void serial_enable_dlab(int port){
    serial_enable_bits(port, SERIAL_LCR, 0x80);
}

L4_INLINE void serial_disable_dlab(int port){
    /* reset dlab, reset break, no parity, 1 stop, 8 data */
    serial_disable_bits(port, SERIAL_LCR, 0x80);
}

L4_INLINE void serial_set_divisor(int port, l4_uint16_t divisor){
    serial_enable_dlab(port);
    serial_set(port, SERIAL_DLL, divisor&0xff);
    serial_set(port, SERIAL_DLM, divisor>>8);
    serial_disable_dlab(port);
}

L4_INLINE l4_uint8_t serial_enable_EFR(int port){
    l4_uint8_t oldval;
    oldval = serial_get(port, SERIAL_LCR);
    serial_set(port, SERIAL_LCR, 0xbf);
    return oldval;
}
L4_INLINE void serial_disable_EFR(int port, l4_uint8_t oldval){
    /* set back to 8n1 */
    serial_set(port, SERIAL_LCR, oldval);
}

L4_INLINE void serial_sendchar(int port, char c){
    serial_set(port, SERIAL_THR, c);
}


/* init the serial line belonging to port with 115000 baud, 8n1,
   irq's on incoming and delivery. No pic programming will take place. */
L4_INLINE void serial_setup(int port){
    /* select bitrate with 115.200 bps */
    serial_set_divisor(port, 1);

    /* enable irq on xmit and rcv */
    serial_set(port, SERIAL_IER, 3);

    /* enable master-irq-bit, set dtr and rts */
    serial_enable_bits(port, SERIAL_MCR, 0x08|0x02|0x01);

#if 1
    /* Enable FIFO mode. Set DMA mode 1 (blocked IRQ), medium  trigger levels,
     * no reset, enable FIFP */
    serial_set(port, SERIAL_FCR, 0x59);
    /* and do it twice */
    serial_set(port, SERIAL_FCR, 0x59);

    /* set automatic RTS and CTS flow control */
    if(0){
	l4_uint8_t val;
	val=serial_enable_EFR(port);
	serial_enable_bits(port, SERIAL_EFR, 0xC0);
	serial_disable_EFR(port, val);
    }
#endif
}

#endif
