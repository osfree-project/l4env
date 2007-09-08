/*
 * \brief   Hardware-interrupt subsystem
 * \author  Thomas Friebel <tf13@os.inf.tu-dresden.de>
 * \author  Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * \date    2007-01-22
 *
 * FIXME could intloop_param freed after startup?
 * FIXME use consume flag to indicate IRQ was handled
 */

#include <l4/dde/ddekit/interrupt.h>
#include <l4/dde/ddekit/semaphore.h>
#include <l4/dde/ddekit/thread.h>
#include <l4/dde/ddekit/memory.h>

#include <l4/omega0/client.h>
#include <l4/log/l4log.h>

#include <stdio.h>

#define DEBUG_INTERRUPTS 0


/*
 * Internal type for interrupt loop parameters
 */
struct intloop_params
{
	unsigned          irq;       /* irq number */
	int               shared;    /* irq sharing supported? */
	void(*thread_init)(void *);  /* thread initialization */
	void(*handler)(void *);      /* IRQ handler function */
	void             *priv;      /* private token */ 
	ddekit_sem_t     *started;

	int               start_err;
};

/**
 * Interrupt service loop
 *
 */
static void intloop(void *arg)
{
	struct intloop_params *params = arg;

	omega0_irqdesc_t desc = { .i = 0 };
	omega0_request_t req  = { .i = 0 };
	int o0handle;

	/* setup interrupt descriptor */
	desc.s.num = params->irq + 1;
	desc.s.shared = params->shared;

	/* allocate irq */
	o0handle = omega0_attach(desc);
	if (o0handle < 0) {
		/* inform thread creator of error */
		/* XXX does omega0 error code have any meaning to DDEKit users? */
		params->start_err = o0handle;
		ddekit_sem_up(params->started);
		return;
	}

	/* after successful initialization call thread_init() before doing anything
	 * else here */
	if (params->thread_init) params->thread_init(params->priv);

	/* save handle + inform thread creator of success */
	params->start_err = 0;
	ddekit_sem_up(params->started);

	/* prepare request structure */
	req.s.param   = params->irq + 1;
	req.s.wait    = 1;
	req.s.consume = 0;
	req.s.unmask  = 1;

	while (1) {
		int err;

		/* wait for int */
		err = omega0_request(o0handle, req);
		if (err != params->irq + 1)
			LOG("omega0_request returned %d, %d (irq+1) expected", err, params->irq + 1);

		LOGd(DEBUG_INTERRUPTS, "received irq 0x%X", err - 1);

		/* unmask only the first time */
		req.s.unmask = 0;

		/* call registered handler function */
		params->handler(params->priv);
		LOGd(DEBUG_INTERRUPTS, "after irq handler");
	}
}


/**
 * Attach to hardware interrupt
 *
 * \param irq          IRQ number to attach to
 * \param shared       set to 1 if interrupt sharing is supported; set to 0
 *                     otherwise
 * \param thread_init  called just after DDEKit internal init and before any
 *                     other function
 * \param handler      IRQ handler for interrupt irq
 * \param priv         private token (argument for thread_init and handler)
 *
 * \return pointer to interrupt thread created
 */
ddekit_thread_t *ddekit_interrupt_attach(int irq, int shared,
                                         void(*thread_init)(void *),
                                         void(*handler)(void *), void *priv)
{
	struct intloop_params *params;
	ddekit_thread_t *thread;
	char thread_name[10];

	/* initialize info structure for interrupt loop */
	params = ddekit_simple_malloc(sizeof(*params));
	if (!params) return NULL;

	params->irq         = irq;
	params->thread_init = thread_init;
	params->handler     = handler;
	params->priv        = priv;
	params->started     = ddekit_sem_init(0);
	params->start_err   = 0;
	params->shared      = shared;

	/* construct name */
	snprintf(thread_name, 10, "irq%02X", irq);

	/* create interrupt loop thread */
	thread = ddekit_thread_create(intloop, params, thread_name);
	if (!thread) {
		ddekit_simple_free(params);
		return NULL;
	}

	/* wait for intloop initialization result */
	ddekit_sem_down(params->started);
	ddekit_sem_deinit(params->started);
	if (params->start_err) {
		ddekit_simple_free(params);
		return NULL;
	}

	return thread;
}

