#include <l4/dde/ddekit/thread.h>
#include <l4/dde/ddekit/condvar.h>
#include <l4/dde/ddekit/panic.h>
#include <l4/dde/ddekit/memory.h>
#include <l4/dde/ddekit/printf.h>

#include <l4/dde/dde.h>
#include <l4/thread/thread.h>
#include <l4/log/l4log.h>
#include <l4/sys/syscalls.h>
#include <l4/util/rdtsc.h>

#include <stdio.h>
#include <string.h>

#define DDEKIT_THREAD_STACK_SIZE 0x2000 /* 8 KB */

static struct ddekit_slab *ddekit_stack_slab = NULL;

struct ddekit_thread {
	l4thread_t l4thread;
	void *data;
	void *stack;
	ddekit_condvar_t *sleep_cv;
	const char *name;
};

/**
 * The thread-local-storage key for the BSD struct thread.
 */
static int tlskey_thread;

struct startup_args {
	void (*fun)(void *);
	void *arg;
	const char *name;
};

ddekit_thread_t *ddekit_thread_setup_myself(const char *name) {
	ddekit_thread_t *td;
	int namelen = strlen(name);
	char *pname;
	
	td = ddekit_simple_malloc(sizeof(*td) + (namelen+1));
	pname = (char *) td + sizeof(*td);

	td->data=NULL;
	td->sleep_cv = ddekit_condvar_init();
	td->l4thread = l4thread_myself();
	td->name = pname;

	strcpy(pname, name);

	l4thread_data_set_current(tlskey_thread, td);

	return td;
}

static void ddekit_thread_startup(void *arg) {
	struct startup_args su;
	ddekit_thread_t *td;

	/* copy arg to su so that it can bee freed by caller */
	su = *((struct startup_args*)arg);

	/* init dde thread structure */
	td = ddekit_thread_setup_myself(su.name);
	/* inform caller of initialization */
	l4thread_started(td);

	/* call thread routine */
	su.fun(su.arg);
}

ddekit_thread_t *ddekit_thread_create(void (*fun)(void *), void *arg, const char *name) {
	struct startup_args su;
	ddekit_thread_t *td;
	l4thread_t l4td;
	char l4name[20];
	void *stack;
	
	su.fun  = fun;
	su.arg  = arg;
	su.name = name;
	
	snprintf(l4name, 20, ".%s", name);
	
	stack  = ddekit_slab_alloc(ddekit_stack_slab);
	
	
	l4td = l4thread_create_long(L4THREAD_INVALID_ID, ddekit_thread_startup, l4name,
                                    (l4_addr_t) stack + (DDEKIT_THREAD_STACK_SIZE-1 )* sizeof (void *),
                                    DDEKIT_THREAD_STACK_SIZE,
                                    L4THREAD_DEFAULT_PRIO, &su, L4THREAD_CREATE_SYNC);
			
	if (l4td < 0)
		ddekit_panic("error creating thread");

	td = (ddekit_thread_t*) l4thread_startup_return(l4td);
	
	td->stack = stack;
	
	return td;
}

ddekit_thread_t *ddekit_thread_myself(void) {
	return (ddekit_thread_t *) l4thread_data_get_current(tlskey_thread);
}

void ddekit_thread_set_data(ddekit_thread_t *thread, void *data) {
	thread->data = data;
}

void ddekit_thread_set_my_data(void *data) {
	ddekit_thread_set_data(ddekit_thread_myself(), data);
}

void *ddekit_thread_get_data(ddekit_thread_t *thread) {
	return thread->data;
}

void *ddekit_thread_get_my_data() {
	return ddekit_thread_get_data(ddekit_thread_myself());
}

void ddekit_thread_msleep(unsigned long msecs) {
	l4thread_sleep(msecs);
}

void ddekit_thread_usleep(unsigned long usecs) {
	l4_busy_wait_us(usecs);
}


void ddekit_thread_nsleep(unsigned long nsecs) {
	l4_busy_wait_ns(nsecs);
}

void ddekit_thread_sleep(ddekit_lock_t *lock) {
	ddekit_thread_t *td;

	td = ddekit_thread_myself();

	ddekit_condvar_wait(td->sleep_cv, lock);
}

void  ddekit_thread_wakeup(ddekit_thread_t *td) {
	ddekit_condvar_signal(td->sleep_cv);
}

void  ddekit_thread_exit() {
	ddekit_thread_t *td;

	td = ddekit_thread_myself();
	
	l4thread_exit();

	ddekit_slab_free(ddekit_stack_slab ,td->stack);
	
}

void ddekit_thread_terminate(ddekit_thread_t *t)
{
	l4thread_shutdown(t->l4thread);
}

const char *ddekit_thread_get_name(ddekit_thread_t *thread) {
	return thread->name;
}

int ddekit_thread_get_id(ddekit_thread_t *t)
{
	return t->l4thread;
}

void ddekit_thread_schedule(void)
{
	l4_yield();
}

void ddekit_yield(void)
{
	l4_yield();
}

void ddekit_init_threads() {
	/* register TLS key for pointer to dde thread structure */
	tlskey_thread = l4thread_data_allocate_key();
	
	/* setup dde part of thread data */
	ddekit_thread_setup_myself("main");
	
 	/* create slab for stacks */
	ddekit_stack_slab = ddekit_slab_init(DDEKIT_THREAD_STACK_SIZE, 1);
}
