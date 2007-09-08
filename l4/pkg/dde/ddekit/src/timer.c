#include <l4/dde/ddekit/timer.h>
#include <l4/dde/ddekit/thread.h>
#include <l4/dde/ddekit/printf.h>
#include <l4/dde/ddekit/panic.h>
#include <l4/dde/ddekit/assert.h>
#include <l4/dde/ddekit/memory.h>

#include <l4/thread/thread.h>
#include <l4/lock/lock.h>
#include <l4/env/errno.h>
#include <l4/generic_io/libio.h>
#include <l4/sys/ipc.h>
#include <l4/util/rdtsc.h>

#define	__DEBUG	0

#define DDEKIT_TIMER_MAGIC	0xABCD1234

/* Just to remind BjoernD of what this is:
 * HZ = clock ticks per second
 * jiffies = clock ticks counter.
 *
 * So, if someone schedules a timeout to expire in 2 seconds,
 * this expires date will be in jiffies + 2 * HZ.
 */
extern volatile unsigned long jiffies;
extern unsigned long HZ;

typedef struct _timer
{
	struct _timer      *next;
	void               (*fn)(void *);
	void               *args;
	unsigned long 	   expires;
	int                id;
} ddekit_timer_t;

static ddekit_timer_t *timer_list  = NULL;
static l4lock_t       timer_lock   = L4LOCK_UNLOCKED;
static l4_threadid_t  timer_thread = L4_NIL_ID;
static ddekit_thread_t *timer_thread_ddekit = NULL;

static int timer_id_ctr = 0;


static void dump_list(char *msg)
{
#if __DEBUG
	ddekit_timer_t *l = timer_list;

	ddekit_printf("-=-=-=-= %s =-=-=-\n", msg);
	while (l) {
		ddekit_printf("-> %d (%lld)\n", l->id, l->expires);
		l = l->next;
	}
	ddekit_printf("-> NULL\n");
	ddekit_printf("-=-=-=-=-=-=-=-\n");
#endif
}


/** Notify the timer thread there is a new timer at the beginning of the
 *  timer list.
 */
static inline void __notify_timer_thread(void)
{
	int err;
	l4_msgdope_t result;
	
	/* Do not notify if there is no timer thread.
	 * XXX: Perhaps we should better assert that there is a timer
	 *      thread before allowing users to add a timer.
	 */
	if (l4_is_nil_id(timer_thread))
		return;

	err = l4_ipc_send(timer_thread, L4_IPC_SHORT_MSG,
	                  DDEKIT_TIMER_MAGIC,
	                  DDEKIT_TIMER_MAGIC,
	                  L4_IPC_SEND_TIMEOUT_0,
	                  &result);

	/* This will either return 0 if notify succeeded or return
	 * SETIMEOUT if the send timeout failed. Everything else should
	 * make us nervous.
	 */
	Assert((err == 0 || err == L4_IPC_SETIMEOUT));
}


int ddekit_add_timer(void (*fn)(void *), void *args, unsigned long timeout)
{
	ddekit_timer_t *it;
	ddekit_timer_t *t = ddekit_simple_malloc(sizeof(ddekit_timer_t));

	Assert(t);

	/* fill in values */
	t->fn      = fn;
	t->args    = args;
	t->expires = timeout;
	t->next    = NULL;

	l4lock_lock(&timer_lock);

	t->id         = timer_id_ctr++;

	/* the easy case... */
	if (timer_list == NULL || timer_list->expires >= t->expires) {
		t->next    = timer_list;
		timer_list = t;
	}
	else { /* find where to insert */
		it = timer_list;
		while (it->next && it->next->expires < t->expires)
			it = it->next;

		if (it->next) { /* insert somewhere in the middle */
			t->next  = it->next;
			it->next = t;
		}
		else /* we append */
			it->next = t;
	}

	/* if we modified the first entry of the list, it is
	 * necessary to notify the timer thread.
	 */
	if (t == timer_list) {
		Assert(!l4_is_nil_id(timer_thread));
		__notify_timer_thread();
	}

	l4lock_unlock(&timer_lock);

	dump_list("after add");
	
	return t->id;
}


int ddekit_del_timer(int timer)
{
	ddekit_timer_t *it, *it_next;
	int ret = -1;

	l4lock_lock(&timer_lock);

	/* no timer? */
	if (!timer_list) {
		ret = -2;
		goto out;
	}

	/* removee is first item, simply delete it */
	if (timer_list->id == timer) {
		it = timer_list->next;
		ret = timer_list->id;
		ddekit_simple_free(timer_list);
		timer_list = it;

		/* XXX: Yes, we could notify the timer thread here, so that it can
		 *      recalculate its sleep to now. However, this will require an
		 *      unnecessary IPC here. The timer thread will wake up in any 
		 *      case, find out that there is no timer for now, and return
		 *      to sleep.
		 */
		goto out;
	}

	it = timer_list;
	it_next = it->next;

	/* more difficult if removee is somewhere in
	 * the middle of the list
	 */
	while (it_next) {
		if (it_next->id == timer) {
			it->next = it->next->next;
			ret = it_next->id;
			ddekit_simple_free(it_next);
			goto out;
		}
		it = it->next;
		it_next = it->next;
	}

out:
	l4lock_unlock(&timer_lock);

	dump_list("after del");
	
	return ret;
}


/** Check whether a timer with a given ID is still pending.
 *
 * \param timer	Timer ID to check for.
 * \return 0 if not pending
 *         1 if timer is pending
 */
int ddekit_timer_pending(int timer)
{
	ddekit_timer_t *t = NULL;
	int r = 0;

	l4lock_lock(&timer_lock);

	t = timer_list;
	while (t) {
		if (t->id == timer) {
			r = 1;
			break;
		}
		t = t->next;
	}

	l4lock_unlock(&timer_lock);

	return r;
}


/** Get the next timer function to run.
 *
 * \return NULL	    if no timer is to be run now
 * 		   != NULL  next timer to execute
 */
static ddekit_timer_t *get_next_timer(void)
{
	ddekit_timer_t *t = NULL;

	/* This function must be called with the timer_lock held. */
	Assert(l4_thread_equal(l4thread_l4_id(l4lock_owner(&timer_lock)), 
				           timer_thread));

	if (timer_list &&
	   (timer_list->expires <= jiffies)) {
		t = timer_list;
		timer_list = timer_list->next;
	}

	return t;
}


/** Let the timer thread sleep for a while. 
 *
 * \param	to	L4 IPC timeout
 *
 * \return	1 if IPC timed out
 * 			0 if message received -> recalc timeout
 */
static inline int __timer_sleep(l4_timeout_t to)
{
	l4_threadid_t any = L4_NIL_ID;
	l4_threadid_t me = l4_myself();
	l4_umword_t dummy;
	l4_msgdope_t res;

	int err = 0;
	
	l4lock_unlock(&timer_lock);

	do {
#if 0
		ddekit_printf("Going to sleep for %lu µs (%lu ms)\n",
		              l4util_l4to2micros(to.to.rcv_man, to.to.rcv_exp),
					  (unsigned long )l4util_l4to2micros(to.to.rcv_man, to.to.rcv_exp) * 1000 / HZ);
#endif
		err = l4_ipc_wait(&any, L4_IPC_SHORT_MSG, &dummy, &dummy, to, &res);
#if 0
		ddekit_printf("ipc from "l4util_idfmt"\n", l4util_idstr(any));
		ddekit_printf("me: "l4util_idfmt"\n", l4util_idstr(me));
		ddekit_printf("err: %x\n", err);
#endif
	} while (!err && !l4_task_equal(me, any));

	l4lock_lock(&timer_lock);

	return (err ? 1 : 0);
}


static void ddekit_timer_thread(void *arg)
{
	timer_thread_ddekit = ddekit_thread_setup_myself("ddekit_timer");

	l4thread_started(0);

	l4lock_lock(&timer_lock);
	while (1) {
		ddekit_timer_t     *timer = NULL;
		l4_timeout_t       to     = L4_IPC_NEVER;

		if (timer_list) {
#if 0
			int jdiff = timer_list->expires - jiffies;
			ddekit_printf("\033[31mscheduling new timeout.\033[0m\n");
			ddekit_printf("\033[31mjiffies diff = %ld (%d s)\033[0m\n", jdiff, jdiff/HZ);
#endif
			unsigned long usec = (timer_list->expires - jiffies) * 1000000 / HZ;
			to = l4_timeout(L4_IPC_TIMEOUT_NEVER, l4util_micros2l4to(usec));
		}

		__timer_sleep(to);

		while ((timer = get_next_timer()) != NULL) {
			l4lock_unlock(&timer_lock);
			//ddekit_printf("doing timer fn @ %p\n", timer->fn);
			timer->fn(timer->args);
			l4lock_lock(&timer_lock);
			ddekit_simple_free(timer);
		}
	}
}

ddekit_thread_t *ddekit_get_timer_thread()
{
	return timer_thread_ddekit;
}


void ddekit_init_timers(void)
{
	l4_tsc_init(L4_TSC_INIT_AUTO);
	/* XXX this module needs HZ and jiffies to work - so l4io info page must be mapped */
	timer_thread = l4thread_l4_id( l4thread_create_named(ddekit_timer_thread,
	                                "ddekit_timer", 0,
	                                L4THREAD_CREATE_SYNC));
}
