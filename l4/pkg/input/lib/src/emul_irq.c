/* $Id$ */
/*****************************************************************************/
/**
 * \file   input/lib/src/emul_irq.c
 * \brief  L4INPUT: Linux IRQ handling emulation
 *
 * \date   11/20/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <stdio.h>
#include <string.h>
#include <l4/sys/ipc.h>
#include <l4/sys/types.h>
#include <l4/util/irq.h>
#ifndef ARCH_arm
#include <l4/util/port_io.h>
#endif
#include <l4/util/macros.h>
#include <l4/util/thread.h>
#include <l4/omega0/client.h>
#include <l4/rmgr/librmgr.h>
#include <l4/env/errno.h>
#include <l4/thread/thread.h>

/* Linux */
#include <linux/interrupt.h>

#include "internal.h"

#define DEBUG_IRQ         0
#define DEBUG_IRQ_VERBOSE 0

/* return within 50ms even if there was no interrupt */
#define IRQ_TIMEOUT	  l4_ipc_timeout(0,0,781,6)

/* INTERRUPT HANDLING EMULATION */

static struct irq_desc {
	int active;
	int num;
	void *cookie;
	l4thread_t t;
	irqreturn_t (*handler)(int, void *, struct pt_regs *);
} handlers[NR_IRQS];

/* We use omega0 mode per default -- support for native IRQs is only an interim
 * hack for ARM. */
static int use_omega0 = 0;

static int irq_prio   = L4THREAD_DEFAULT_PRIO;

/* OMEGA0 STUFF */

#define OM_MASK    0x00000001
#define OM_UNMASK  0x00000002
#define OM_CONSUME 0x00000004
#define OM_AGAIN   0x00000008

/** Attach IRQ line.
 *
 * \param  irq           IRQ number
 * \retval handle        IRQ handle
 *
 * \return 0 on success (attached interrupt), -1 if attach failed.
 */
static inline int __omega0_attach(unsigned int irq, int *handle)
{
#ifndef ARCH_arm
	omega0_irqdesc_t irqdesc;

	/* setup irq descriptor */
	irqdesc.s.num = irq + 1;
	irqdesc.s.shared = 0;

	/* attach IRQ */
	*handle = omega0_attach(irqdesc);
	if (*handle < 0)
		return -1;
	else
#endif
		return 0;
}

/** Wait for IRQ notification.
 *
 * \param  irq           IRQ number
 * \param  handle        IRQ line handle
 * \param  flags         Flags:
 *                       - \c OM_MASK    request IRQ mask
 *                       - \c OM_UNMASK  request IRQ unmask
 *                       - \c OM_CONSUME IRQ consumed
 * \return               0 for success
 *                       L4_IPC_RETIMEOUT if no IRQ was received (timeout)
 */
static inline int __omega0_wait(unsigned int irq, int handle,
                                unsigned int flags)
{
#ifndef ARCH_arm
	omega0_request_t request = { .i=0 };
	int ret;

	/* if again, don't perform any other actions */
	if (flags & OM_AGAIN)
	  flags &= ~OM_AGAIN;

	/* setup omega0 request */
	request.s.param   = irq + 1;
	request.s.wait    = 1;
	request.s.consume = (flags & OM_CONSUME) ? 1 : 0;
	request.s.mask    = (flags & OM_MASK)    ? 1 : 0;
	request.s.unmask  = (flags & OM_UNMASK)  ? 1 : 0;
	request.s.again   = (flags & OM_AGAIN)   ? 1 : 0;

	/* wait for interrupt */
	ret = omega0_request_timeout(handle, request, IRQ_TIMEOUT);
	if (ret == -L4_IPC_RETIMEOUT)
		return L4_IPC_RETIMEOUT;
	if (ret != (irq + 1))
		Panic("error waiting for interrupt (error %08x)", ret);
#endif
	return 0;
}

/** IRQ HANDLER THREAD.
 *
 * \param irq_desc  IRQ handling descriptor (IRQ number and handler routine)
 */
static void __irq_handler(struct irq_desc *irq_desc)
{
	unsigned int irq = irq_desc->num; /* save irq number */
	void *cookie = irq_desc->cookie;  /* save cookie/dev_id pointer */

	int ret, error = L4_IPC_RETIMEOUT, irq_handle = -1;
	l4_threadid_t irq_id;
	l4_umword_t dw0, dw1;
	l4_msgdope_t result;
	unsigned int om_flags;

	/* get permission for attaching to IRQ */
	if (use_omega0) {
		ret = __omega0_attach(irq, &irq_handle);
		if (ret < 0)
			Panic("failed to attach IRQ %d at omega0!\n", irq);
		else
			error = L4_IPC_RETIMEOUT;
	} else {
		/* attach to IRQ */
		if (l4_is_invalid_id(irq_id = l4util_attach_interrupt(irq)))
			Panic("__irq_handler(): "
			      "can't attach to irq 0x%02x, giving up...\n", irq);

		/* read spurious interrupts */
		for (;;) {
			error = l4_ipc_receive(irq_id,
			                       L4_IPC_SHORT_MSG, &dw0, &dw1,
			                       l4_ipc_timeout(0, 0, 1, 0), &result);
			if (error == L4_IPC_RETIMEOUT)
				break;
		}
	}

	/* we are up */
	ret = l4thread_started(NULL);

#if DEBUG_IRQ
	printf("emul_irq.c: __irq_handler "l4util_idfmt" running (%p)\n",
	       l4util_idstr(l4thread_l4_id(l4thread_myself())),
	       irq_desc->handler);
#endif

	if (ret || (error != L4_IPC_RETIMEOUT))
		Panic("IRQ thread startup failed!\n");

	om_flags = OM_UNMASK;
	for (;;) {
		if (use_omega0)
			error = __omega0_wait(irq, irq_handle, om_flags);
		else
			error = l4_ipc_receive(irq_id,
			                       L4_IPC_SHORT_MSG, &dw0, &dw1,
			                       IRQ_TIMEOUT, &result);

		switch (error) {
		case 0:
#if DEBUG_IRQ_VERBOSE
			printf("emul_irq.c: got IRQ %d\n", irq);
#endif
			if (irq_desc->active)
				irq_desc->handler(irq, cookie, NULL);
			if (use_omega0)
				om_flags = 0;
			break;

		case L4_IPC_RETIMEOUT:
			if (irq_desc->active)
				irq_desc->handler(irq, cookie, NULL);
			if (use_omega0)
				om_flags = OM_AGAIN;
			break;

		default:
			Panic("error receiving irq");
			break;
		}
	}
}

/** Request Interrupt.
 * \ingroup grp_irq
 *
 * \param irq      interrupt number
 * \param handler  interrupt handler -> top half
 * \param ...
 * \param cookie   cookie pointer passed back
 *
 * \return 0 on success; error code otherwise
 */
int request_irq(unsigned int irq,
                irqreturn_t (*handler)(int, void *, struct pt_regs *),
                unsigned long flags, const char *devname, void *cookie)
{
	l4thread_t irq_tid;
	char buf[7];

	if (irq >= NR_IRQS)
		return -L4_EINVAL;
	if (!handler)
		return -L4_EINVAL;

	handlers[irq].num = irq;
	handlers[irq].cookie = cookie;
	handlers[irq].handler = handler;

	handlers[irq].active = 1;

	if(handlers[irq].t) {
#if DEBUG_IRQ
		printf("emul_irq.c: reattached to irq %d\n", irq);
#endif
		return 0;
	}

	sprintf(buf, ".irq%.2X", irq);
	/* create IRQ handler thread */
	irq_tid = l4thread_create_long(L4THREAD_INVALID_ID,
	                               (l4thread_fn_t) __irq_handler,
	                               buf,
	                               L4THREAD_INVALID_SP,
	                               L4THREAD_DEFAULT_SIZE,
	                               irq_prio,
	                               (void *) &handlers[irq], 
	                               L4THREAD_CREATE_SYNC);

#if DEBUG_IRQ
	printf("emul_irq.c: attached to irq %d\n", irq);
#endif

	handlers[irq].t = irq_tid;

	return 0;
}

/** Release Interrupt.
 *
 * \param irq     interrupt number
 * \param cookie  cookie pointer passed back
 *
 * \todo Implementation.
 */
void free_irq(unsigned int irq, void *cookie)
{
	handlers[irq].active = 0;

#if DEBUG_IRQ
	printf("emul_irq.c: attempt to free irq %d ("l4util_idfmt")\n", irq,
	       l4util_idstr(l4thread_l4_id(handlers[irq].t)));
#endif
}

/* INTERRUPT EMULATION INITIALIZATION */
void l4input_internal_irq_init(int prio)
{
#ifdef ARCH_arm
	use_omega0 = 0;
#else
	use_omega0 = 1;
#endif
	irq_prio   = prio;

	memset(&handlers, 0, sizeof(handlers));
}
