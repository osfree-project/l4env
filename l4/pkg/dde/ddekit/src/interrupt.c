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
#include <l4/dde/ddekit/panic.h>

#include <l4/omega0/client.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>

#include <stdio.h>

#define DEBUG_INTERRUPTS  0

#define MAX_INTERRUPTS   32

#define BLOCK_IRQ         0

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

static struct
{
	int               handle_irq; /* nested irq disable count */
	ddekit_sem_t     *irqsem;     /* synch semaphore */
	ddekit_thread_t  *irq_thread; /* thread ID for detaching from IRQ later on */
	omega0_irqdesc_t  irq_desc;   /* Omega0-style IRQ descriptor */
} ddekit_irq_ctrl[MAX_INTERRUPTS];


static void ddekit_irq_exit_fn(l4thread_t thread, void *data)
{
	int idx = (int)data;

	// * detach from IRQ
	omega0_detach(ddekit_irq_ctrl[idx].irq_desc);
}

L4THREAD_EXIT_FN_STATIC(exit_fn, ddekit_irq_exit_fn);


/**
 * Interrupt service loop
 *
 */
static void intloop(void *arg)
{
	struct intloop_params *params = arg;

    omega0_request_t req  = { .i = 0 };
	int o0handle;
	int my_index = params->irq;
    omega0_irqdesc_t *desc = &ddekit_irq_ctrl[my_index].irq_desc;

	/* setup interrupt descriptor */
	desc->s.num = params->irq + 1;
	desc->s.shared = params->shared;

	/* allocate irq */
	o0handle = omega0_attach(*desc);
	if (o0handle < 0) {
		/* inform thread creator of error */
		/* XXX does omega0 error code have any meaning to DDEKit users? */
		params->start_err = o0handle;
		ddekit_sem_up(params->started);
		return;
	}

	/* 
	 * Setup an exit fn. This will make sure that we clean up everything,
	 * before shutting down an IRQ thread.
	 */
	if (l4thread_on_exit(&exit_fn, (void *)my_index) < 0)
		ddekit_panic("Could not set exit handler for IRQ fn.");

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

		/* only call registered handler function, if IRQ is not disabled */
		ddekit_sem_down(ddekit_irq_ctrl[my_index].irqsem);
		if (ddekit_irq_ctrl[my_index].handle_irq > 0) {
			LOGd(DEBUG_INTERRUPTS, "not handling IRQ %x, because it is disabled.", my_index);
			params->handler(params->priv);
		}
		ddekit_sem_up(ddekit_irq_ctrl[my_index].irqsem);
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

	ddekit_irq_ctrl[irq].handle_irq = 1; /* IRQ nesting level is initially 1 */
	ddekit_irq_ctrl[irq].irq_thread = thread;
	ddekit_irq_ctrl[irq].irqsem     = ddekit_sem_init(1);


	/* wait for intloop initialization result */
	ddekit_sem_down(params->started);
	ddekit_sem_deinit(params->started);
	if (params->start_err) {
		ddekit_simple_free(params);
		return NULL;
	}

	return thread;
}

/**
 * Detach from interrupt by disabling it and then shutting down the IRQ
 * thread.
 */
void ddekit_interrupt_detach(int irq)
{
	ddekit_interrupt_disable(irq);
	ddekit_thread_terminate(ddekit_irq_ctrl[irq].irq_thread);
}


void ddekit_interrupt_disable(int irq)
{
	ddekit_sem_down(ddekit_irq_ctrl[irq].irqsem);
	--ddekit_irq_ctrl[irq].handle_irq;
	ddekit_sem_up(ddekit_irq_ctrl[irq].irqsem);
}


void ddekit_interrupt_enable(int irq)
{
	ddekit_sem_down(ddekit_irq_ctrl[irq].irqsem);
	++ddekit_irq_ctrl[irq].handle_irq;
	ddekit_sem_up(ddekit_irq_ctrl[irq].irqsem);
}
