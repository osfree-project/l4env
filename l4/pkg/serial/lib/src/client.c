/*!
 * \file   serial/lib/src/client.c
 * \brief  Implementation of the serial lib
 *
 * \date   10/21/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/util/util.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/log/l4log.h>
#include <l4/sys/kdebug.h>
#include <l4/util/port_io.h>
#include <l4/util/thread.h>
#include <l4/util/irq.h>
#include <l4/util/macros.h>
#include <l4/rmgr/librmgr.h>
#include <stdio.h>
#include <l4/serial/serial.h>
#include "serial_op.h"

enum l4serial_cmds{
    L4SERIAL_SEND,		/* request to send a string. len in upper
				 * 24 bit. */
    L4SERIAL_RECEIVE,		/* request for the next received byte. */
    L4SERIAL_ANS_RX,		/* received byte in dw1 */
    L4SERIAL_ANS_TX,		/* string was sent */
    L4SERIAL_ANS_BUSY,		/* another string is beeing sent currently */
};

static unsigned stack[4096];
static int port;
static int irq;
static l4_threadid_t start_t;
static l4_threadid_t handler_t = L4_INVALID_ID;

typedef struct{
    unsigned cmd:8;
    unsigned len:24;
} l4serial_cmd_t;

static void irq_unmask(int irq){
    l4util_cli();
    if(irq<8){
	l4util_out8(l4util_in8(0x21) & ~(1<<irq), 0x21);  
    } else {
	l4util_out8(l4util_in8(0xa1) & ~(1<<(irq-8)), 0xa1);
    }
    l4util_sti();
}

/*!\brief Send an answer to client, invalidate client, wait for next */
static int client_reply_wait(l4_threadid_t *client, l4_umword_t ans_0,
			     l4_umword_t ans_1,
			     l4_threadid_t *new_client, l4_umword_t *new_0,
			     l4_umword_t *new_1){
    l4_msgdope_t result;
    int err = l4_ipc_reply_and_wait(*client, L4_IPC_SHORT_MSG, ans_0, ans_1,
				    new_client, L4_IPC_SHORT_MSG, new_0, new_1,
				    L4_IPC_SEND_TIMEOUT_0, &result);
    if(client!=new_client) *client = L4_INVALID_ID;
    return err;
}

/*!\brief Send an answer to client and invalidate client afterwards */
static int client_send_inv(l4_threadid_t *client, l4_umword_t ans_0,
			   l4_umword_t ans_1){
    l4_msgdope_t result;
    int err = l4_ipc_send(*client, L4_IPC_SHORT_MSG, ans_0, ans_1,
			  L4_IPC_SEND_TIMEOUT_0, &result);
    *client = L4_INVALID_ID;
    return err;
}

static void serial_irq_fn(void){
    l4_threadid_t irq_t;		/* serial IRQ thread number */
    l4_threadid_t tx_t = L4_INVALID_ID;	/* client waiting for tx to finish */
    l4_threadid_t rx_t = L4_INVALID_ID;	/* client waiting for an rx-event */
    l4_threadid_t thread;		/* current request */
    l4_msgdope_t result;
    l4_umword_t dw0, dw1;
    unsigned send_len=0;		/* chars still to send */
    unsigned char *send_string=0;	/* ptr to chars to send */
    int err;
    int rx_byte=-1;

    err = rmgr_get_irq(irq);
    irq_t = l4util_attach_interrupt(irq);
    if((err=l4_ipc_send(start_t, L4_IPC_SHORT_MSG,
			l4_is_invalid_id(irq_t)?-2:0, 0,
			L4_IPC_NEVER, &result))!=0){
	printf(l4util_idfmt": Can't send up-IPC to " l4util_idfmt"\n",
	       l4util_idstr(l4_myself()), l4util_idstr(start_t));
	l4_sleep_forever();
    }
    if(l4_is_invalid_id(irq_t)) l4_sleep_forever();
    /* unmask IRQ at PIC */
    irq_unmask(irq);

  l_recv:
    /* We are attached to the irq thread. Wait for requests from some of
     * our threads, or for successful send-IRQs */
    err = l4_ipc_wait(&thread, L4_IPC_SHORT_MSG, &dw0, &dw1,
		      L4_IPC_NEVER, &result);
    while(1){
	while(err){
	    printf("IPC error %x\n", err);
	    err = l4_ipc_wait(&thread, L4_IPC_SHORT_MSG, &dw0, &dw1,
			      L4_IPC_NEVER, &result);
	}
	if(l4_thread_equal(thread, irq_t)){
	    int isr;
	    int lsr;

	    /* Level-triggered: Ack the IRQ at the PIC, and reset interrupt
	     * source then. */
	    l4util_irq_acknowledge(irq);

	    /* Serial IRQ. Read interrupt state, ignoring FIFO state */
	    while(((isr = serial_get(port, SERIAL_ISR)) & 0x6) != 0){
		if(isr & 0x04){	/* RX Interrupt */
		    /* Read rx while lsr indicates so */
		    while(((lsr = serial_get(port, SERIAL_LSR)) & 0x1) != 0){
			/* RX-byte ready. Read char and send to rx-client */
			rx_byte = serial_get(port, SERIAL_RHR);
			if(!l4_is_invalid_id(rx_t)){
			    err = client_send_inv(&rx_t, L4SERIAL_ANS_RX,
						  rx_byte);
			    rx_byte=-1;
			}
		    } /* isr indicated a received byte */
		}
		if(isr & 0x02){	/* TX Interrupt */
		    /* TX is ready. We know this without asking LSR. */
		    /* If another char is avail, send it. Otherwise wake
		     * tx-client. */
		    if(send_len>0){
			serial_sendchar(port, *send_string++);
			send_len--;
		    }
		    if(send_len==0 && !l4_is_invalid_id(tx_t)){
			err = client_send_inv(&tx_t, L4SERIAL_ANS_TX, 0);
		    } /* we sent our last character */
		}  /* isr indicated send-ready */

	    } /* ISR indicates something of interest */

	    /* wait for next IPC message */
	    goto l_recv;
	} /* Serial IRQ */
	switch(((l4serial_cmd_t*)&dw0)->cmd){
	case L4SERIAL_SEND:
	    if(!l4_is_invalid_id(tx_t)){
		err = client_reply_wait(&thread, L4SERIAL_ANS_BUSY, 0,
					&thread, &dw0, &dw1);
		continue;
	    } else {
		/* dw1 contains the address, dw0 the length */
		send_string = (unsigned char*)dw1;
		send_len = ((l4serial_cmd_t*)&dw0)->len;
		if(send_len){
		    tx_t = thread;
		    serial_sendchar(port, *send_string++);
		    send_len--;
		}
		/* first char is sent, the client gets the answer at the
		 * last byte. */
		goto l_recv;
	    }
	case L4SERIAL_RECEIVE:
	    if(rx_byte!=-1){
		err = client_reply_wait(&thread, L4SERIAL_ANS_RX, rx_byte,
					&thread, &dw0, &dw1);
		rx_byte=-1;
		continue;
	    } else {
		rx_t = thread;
		goto l_recv;
	    }
	default:
	    /* ignore invalid request */
	    goto l_recv;
	} /* switch dw0.cmd */
    } /* while(1) */
}

int l4serial_init(int tid, int comport){
    int err;
    l4_msgdope_t result;
    l4_umword_t dw0, dw1;

    switch(comport){
    case 1:
	port = 0x3f8;	// com1
	irq  = 4;	// irq 4
	break;
    case 2:
	port = 0x2f8;	// com2
	irq  = 3;	// irq 3
	break;
    default:
	return -1;
    }

    serial_setup(port);
    start_t = l4_myself();
    handler_t = l4util_create_thread(tid, serial_irq_fn,
				     stack+sizeof(stack));
    if((err = l4_ipc_receive(handler_t, L4_IPC_SHORT_MSG,
			     &dw0, &dw1, L4_IPC_NEVER, &result))!=0){
	handler_t = L4_INVALID_ID;
	return err;
    }
    if(dw0){
	handler_t = L4_INVALID_ID;
    }
    return dw0;
}
    
    
int l4serial_outstring(const char*addr, unsigned len){
    l4_umword_t dw0, dw1;
    l4_msgdope_t result;
    int err;

    if(l4_is_invalid_id(handler_t)) return -1;
    ((l4serial_cmd_t*)&dw0)->cmd = L4SERIAL_SEND;
    ((l4serial_cmd_t*)&dw0)->len = len;
    dw1 = (l4_umword_t)addr;
    err = l4_ipc_call(handler_t, L4_IPC_SHORT_MSG, dw0, (l4_umword_t)addr,
		      L4_IPC_SHORT_MSG, &dw0, &dw1, L4_IPC_NEVER, &result);
    if(err) return err;
    if(dw0 == L4SERIAL_ANS_TX) return 0;
    return -2;
}

int l4serial_readbyte(void){
    l4_umword_t dw0, dw1;
    l4_msgdope_t result;
    int err;

    if(l4_is_invalid_id(handler_t)) return -1;
    ((l4serial_cmd_t*)&dw0)->cmd = L4SERIAL_RECEIVE;
    err = l4_ipc_call(handler_t, L4_IPC_SHORT_MSG, dw0, 0,
		      L4_IPC_SHORT_MSG, &dw0, &dw1, L4_IPC_NEVER, &result);
    if(err) return -2;
    if(dw0 != L4SERIAL_ANS_RX) return -2;
    return dw1;
}
