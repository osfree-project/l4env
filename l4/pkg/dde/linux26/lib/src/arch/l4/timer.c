#include "local.h"

#include <linux/timer.h>

DECLARE_INITVAR(dde26_timer);

/* Definitions from linux/kernel/timer.c */

/*
 * per-CPU timer vector definitions:
 */
#define TVN_BITS (CONFIG_BASE_SMALL ? 4 : 6)
#define TVR_BITS (CONFIG_BASE_SMALL ? 6 : 8)
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)

typedef struct tvec_s {
	struct list_head vec[TVN_SIZE];
} tvec_t;

typedef struct tvec_root_s {
	struct list_head vec[TVR_SIZE];
} tvec_root_t;

struct tvec_t_base_s {
	spinlock_t lock;
	struct timer_list *running_timer;
	unsigned long timer_jiffies;
	tvec_root_t tv1;
	tvec_t tv2;
	tvec_t tv3;
	tvec_t tv4;
	tvec_t tv5;
} ____cacheline_aligned_in_smp;

typedef struct tvec_t_base_s tvec_base_t;

tvec_base_t boot_tvec_bases __attribute__((unused));

static DEFINE_PER_CPU(tvec_base_t *, tvec_bases) __attribute__((unused)) = &boot_tvec_bases;

void fastcall init_timer(struct timer_list *timer) { }

void add_timer(struct timer_list *timer)
{
	CHECK_INITVAR(dde26_timer);
	/* DDE2.6 uses jiffies and HZ as exported from L4IO. Therefore
	 * we just need to hand over the timeout to DDEKit. */
	timer->ddekit_timer_id = ddekit_add_timer((void *)timer->function,
			                                  (void *)timer->data,
											  timer->expires);
}


void add_timer_on(struct timer_list *timer, int cpu)
{
	add_timer(timer);
}


int del_timer(struct timer_list * timer)
{
	CHECK_INITVAR(dde26_timer);
	return (ddekit_del_timer(timer->ddekit_timer_id) >= 0);
}

int del_timer_sync(struct timer_list *timer)
{
	return del_timer(timer);
}


int __mod_timer(struct timer_list *timer, unsigned long expires)
{
	/* XXX: Naive implementation. If we really need to be fast with
	 *      this function, we can implement a faster version inside
	 *      the DDEKit. Bjoern just does not think that this is the
	 *      case.
	 */
	int r;
	
	CHECK_INITVAR(dde26_timer);
	r = del_timer(timer);

	timer->expires = expires;
	add_timer(timer);

	return (r > 0);
}


int mod_timer(struct timer_list *timer, unsigned long expires)
{
	return __mod_timer(timer, expires);
}


int timer_pending(const struct timer_list *timer)
{
	CHECK_INITVAR(dde26_timer);
	/* There must be a valid DDEKit timer ID in the timer field
	 * *AND* it must be pending in the DDEKit.
	 */
	return ((timer->ddekit_timer_id >= 0) 
		    && ddekit_timer_pending(timer->ddekit_timer_id));
}


/**
 * msleep - sleep safely even with waitqueue interruptions
 * @msecs: Time in milliseconds to sleep for
 */
void msleep(unsigned int msecs)
{
	ddekit_thread_msleep(msecs);
}

void msleep_interruptible(unsigned int msecs)
{
	CHECK_INITVAR(dde26_timer);
	current->state = TASK_INTERRUPTIBLE;
	schedule_timeout(msecs);
}

void __const_udelay(unsigned long xloops)
{
	ddekit_thread_usleep(xloops);
}


void __udelay(unsigned long usecs)
{
	__const_udelay(usecs * 0x000010c7); /* 2**32 / 1000000 (rounded up) */
}


void __ndelay(unsigned long nsecs)
{
	__const_udelay(nsecs * 0x00005); /* 2**32 / 1000000000 (rounded up) */
}


void get_random_bytes(void *buf, int nbytes)
{
	*(char *)buf = 0;
}


void l4dde26_init_timers(void)
{
	ddekit_init_timers();

	l4dde26_process_from_ddekit(ddekit_get_timer_thread());

	INITIALIZE_INITVAR(dde26_timer);
}
