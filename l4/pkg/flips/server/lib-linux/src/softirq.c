/* L4 */
#include <l4/util/macros.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/mm.h>
#include <linux/interrupt.h>

/* local */
#include "local.h"

/*  Linux has nothing like this */
#define NR_SOFTIRQS 8

/** initialization flag */
static int _initialized = 0;

/* SOFTIRQ descriptors */
static struct softirq_desc {
	int active;
	int num;                    /**< SOFTIRQ number */
	l4thread_t t;               /**< handler thread */
	l4semaphore_t sema;         /**< producer/consumer semaphore */
	struct softirq_action s;    /**< Linux structure */
} softirq_handlers[NR_SOFTIRQS];

static char *names[NR_SOFTIRQS];

/** RAISE SOFTIRQ (another flavour) */
void __cpu_raise_softirq(unsigned cpu, int nr)
{
	raise_softirq(nr);
}

/** RAISE SOFTIRQ */
void raise_softirq(int nr)
{
	switch (nr) {
	case TASKLET_SOFTIRQ:
	case NET_TX_SOFTIRQ:
	case NET_RX_SOFTIRQ:
		/* softirq _producer_ */
		l4semaphore_up(&softirq_handlers[nr].sema);
		break;

	default:
		LOGd(DEBUG_LIBLX, "softirq %d not supported", nr);
		return;
	}
}

static struct tasklet_head tasklets;
static l4lock_t tasklets_lock = L4LOCK_UNLOCKED;

static void tasklet_action(struct softirq_action *s)
{
	struct tasklet_struct *list;

	l4lock_lock(&tasklets_lock);
	list = tasklets.list;
	tasklets.list = NULL;
	l4lock_unlock(&tasklets_lock);

	while (list != NULL) {
		struct tasklet_struct *t = list;

		list = list->next;

		if (tasklet_trylock(t)) {
			if (atomic_read(&t->count) == 0) {
				clear_bit(TASKLET_STATE_SCHED, &t->state);

				t->func(t->data);

				tasklet_unlock(t);
				continue;
			}
			tasklet_unlock(t);
		}
		l4lock_lock(&tasklets_lock);
		t->next = tasklets.list;
		tasklets.list = t;
		raise_softirq(TASKLET_SOFTIRQ);
		l4lock_unlock(&tasklets_lock);
	}
}

void tasklet_init(struct tasklet_struct *t,
                  void (*func) (unsigned long), unsigned long data)
{
	t->func = func;
	t->data = data;
	t->state = 0;
	atomic_set(&t->count, 0);
}

void tasklet_kill(struct tasklet_struct *t)
{
	if (in_interrupt())
		LOG_Error("Attempt to kill tasklet from interrupt");

	while (test_and_set_bit(TASKLET_STATE_SCHED, &t->state)) {
		do
			/* release CPU on any way (like schedule() does in DDE) */
			l4thread_usleep(1);
		while (test_bit(TASKLET_STATE_SCHED, &t->state));
	}
	tasklet_unlock_wait(t);
	clear_bit(TASKLET_STATE_SCHED, &t->state);
}

void tasklet_schedule(struct tasklet_struct *t)
{
	if (!test_and_set_bit(TASKLET_STATE_SCHED, &t->state)) {
		l4lock_lock(&tasklets_lock);
		t->next = tasklets.list;
		tasklets.list = t;

		/* raise softirq only on new 1st element */
		if (!t->next)
			raise_softirq(TASKLET_SOFTIRQ);
		l4lock_unlock(&tasklets_lock);
	}
}


/*****************************************************************************/

/** INTERNAL: SOFTIRQ THREAD */
static void liblinux_softirq_thread(struct softirq_desc *desc)
{
	int ret, i;
//	int irqc;

	ret = l4dde_process_add_worker();
	Assert(ret == 0);

	++local_bh_count(smp_processor_id());

	ret = l4thread_started(NULL);

	if (ret < 0)
		Panic("SOFTIRQ thread startup failed!");

	LOGd(DEBUG_LIBLX, "liblinux_softirq_thread[%x:%s] running.",
	     l4thread_myself(), names[desc->num]);

	for (;;) {
		/* softirq _consumer_ */
		l4semaphore_down(&desc->sema);

#if 0
		/* XXX --> */
#define INDEX ((index+i+1)%NR_CPUS)
		if (desc->num == NET_RX_SOFTIRQ) {
			int i;
			static int index = 0;
			for (i = 0; i < NR_CPUS; i++)
				if (!list_empty(&softnet_data[INDEX].poll_list)) {
					//LOG("softnet_data[%d].poll_list is not empty (exec %s @ %p)",
					//    INDEX, names[desc->num], desc->s.action);
					if (desc->active) {
						current->processor = INDEX;
						desc->s.action(&desc->s);
					}
					/* will this help ? */
					current->processor = 0;
					index = INDEX;
					break;
				}
		}
#undef INDEX
		/* <-- XXX */
		else if (desc->active) {
			//LOGd(DEBUG_LIBLX, "%s softirq exec (%p)", names[desc->num], desc->s.action);
			desc->s.action(&desc->s);
		}
#else
		switch (desc->num) {
		case NET_RX_SOFTIRQ:
			/* XXX It may happen that high CPU numbers starve. */
			for (i = 0; i < NR_CPUS; i++)
				if (!list_empty(&softnet_data[i].poll_list)) {
					current->processor = i;
					if (desc->active)
						desc->s.action(&desc->s);
					current->processor = 0;
					break;
				}
			break;

		case NET_TX_SOFTIRQ:
			/* XXX It may happen that high CPU numbers starve. */
			for (i = 0; i < NR_CPUS; i++)
				if (softnet_data[i].output_queue ||
				    softnet_data[i].completion_queue) {
					current->processor = i;
//					/* FIXME */ irqc = local_irq_count(smp_processor_id());
//					/* FIXME */ local_irq_count(smp_processor_id()) = 0;
					if (desc->active)
						desc->s.action(&desc->s);
//					/* FIXME */ local_irq_count(smp_processor_id()) = irqc;
					current->processor = 0;
					break;
				}
			break;

		case TASKLET_SOFTIRQ:
			if (desc->active)
				desc->s.action(&desc->s);
			break;

		default:
			Panic("Bug?");
		}
#endif
	}
}

/** REGISTER SOFTIRQ HANDLERS / CREATE THREADS
 */
void open_softirq(int nr, void (*action) (struct softirq_action *),
                  void *data)
{
	int softirq_tid;
	char name[16];

	/* sanity checks */
	Assert(action);
	switch (nr) {
	case TASKLET_SOFTIRQ:
	case NET_TX_SOFTIRQ:
	case NET_RX_SOFTIRQ:
		if (softirq_handlers[nr].active) {
			LOGd(DEBUG_LIBLX, "attempt to inialize softirq %s repeatedly",
			     names[nr]);
			return;
		}
		break;

	default:
		LOG_Error("softirq %d not supported", nr);
		return;
	}

	softirq_handlers[nr].num = nr;
	softirq_handlers[nr].sema = L4SEMAPHORE_LOCKED;
	softirq_handlers[nr].s.action = action;
	softirq_handlers[nr].s.data = data;

	/* create softirq thread */
	snprintf(name, 16, ".sirq%.2X", nr);
	softirq_tid = l4thread_create_long(L4THREAD_INVALID_ID,
	                                   (l4thread_fn_t) liblinux_softirq_thread,
	                                   name,
	                                   L4THREAD_INVALID_SP,
	                                   L4THREAD_DEFAULT_SIZE,
	                                   L4THREAD_DEFAULT_PRIO,
	                                   (void *) &softirq_handlers[nr],
	                                   L4THREAD_CREATE_SYNC);
	if (softirq_tid < 0) {
		LOG_Error("%d creating softirq thread", softirq_tid);
		return;
	}

	softirq_handlers[nr].active = 1;
	softirq_handlers[nr].t = softirq_tid;
}

#if 0
/** GET THREAD ID FOR SOFTIRQ */
l4thread_t liblinux_sofirq_nr2id(int nr)
{
	return softirq_handlers[nr].t;
}
#endif

/** INITIALISATION OF THE SOFTIRQ MODULE 
 *
 * This function must be called once before the ip-stack is started.
 */
int liblinux_softirq_init()
{
	if (_initialized)
		return 0;

	/* init names */
	names[TASKLET_SOFTIRQ] = "TASKLET_SOFTIRQ";
	names[NET_TX_SOFTIRQ] = "NET_TX_SOFTIRQ";
	names[NET_RX_SOFTIRQ] = "NET_RX_SOFTIRQ";

	/* create tasklet handler */
	open_softirq(TASKLET_SOFTIRQ, tasklet_action, NULL);

	++_initialized;
	return 0;
}

