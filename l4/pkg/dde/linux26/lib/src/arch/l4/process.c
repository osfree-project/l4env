#include <l4/dde/dde.h>
#include <l4/dde/linux26/dde26.h>

#include <asm/atomic.h>

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/thread_info.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/vmalloc.h>

#include "local.h"

/*****************************************************************************
 ** Current() implementation                                                **
 *****************************************************************************/
struct thread_info *current_thread_info(void)
{
	dde26_thread_data *cur = (dde26_thread_data *)ddekit_thread_get_my_data();
	return &LX_THREAD(cur);
}

struct task_struct *get_current(void)
{
	return current_thread_info()->task;
}

/*****************************************************************************
 ** PID-related stuff                                                       **
 **                                                                         **
 ** Linux manages lists of PIDs that are handed out to processes so that at **
 ** a later point it is able to determine which task_struct belongs to a    **
 ** certain PID. We implement this with a single list holding the mappings  **
 ** for all our threads.                                                    **
 *****************************************************************************/

LIST_HEAD(_pid_task_list);
ddekit_lock_t _pid_task_list_lock;

/** PID to task_struct mapping */
struct pid2task
{
    struct list_head list;    /**< list data */
    pid_t pid;                /**< PID */
    struct task_struct *ts;   /**< task struct */
};

/** Attach PID to a certain task struct. */
int fastcall attach_pid(struct task_struct *task, enum pid_type type
                        __attribute__((unused)), int nr)
{
	/* Initialize a new pid2task mapping */
	struct pid2task *pt = kmalloc(sizeof(struct pid2task), GFP_KERNEL);
	pt->pid = nr;
	pt->ts = task;

	/* add to list */
	ddekit_lock_lock(&_pid_task_list_lock);
	list_add(&pt->list, &_pid_task_list);
	ddekit_lock_unlock(&_pid_task_list_lock);
	
	return 0;
}

/** Detach PID from a task struct. */
void fastcall detach_pid(struct task_struct *task, enum pid_type type
                                          __attribute__((unused)))
{
	struct list_head *p, *n, *h;

	h = &_pid_task_list;
	
	ddekit_lock_lock(&_pid_task_list_lock);
	/* search for mapping with given task struct and free it if necessary */
	list_for_each_safe(p, n, h) {
		struct pid2task *pt = list_entry(p, struct pid2task, list);
		if (pt->ts == task) {
			list_del(p);
			kfree(pt);
			break;
		}
	}
	ddekit_lock_unlock(&_pid_task_list_lock);
}

struct task_struct *find_task_by_pid_type(int type, int nr)
{
	struct list_head *h, *p;
	h = &_pid_task_list;

	ddekit_lock_lock(&_pid_task_list_lock);
	list_for_each(p, h) {
		struct pid2task *pt = list_entry(p, struct pid2task, list);
		if (pt->pid == nr) {
			ddekit_lock_unlock(&_pid_task_list_lock);
			return pt->ts;
		}
	}
	ddekit_lock_unlock(&_pid_task_list_lock);

	return NULL;
}

/*****************************************************************************
 ** kernel_thread() implementation                                          **
 *****************************************************************************/
/* Struct containing thread data for a newly created kthread. */
struct __kthread_data
{
	int (*fn)(void *);
	void *arg;
};

/** Counter for running kthreads. It is used to create unique names
 *  for kthreads.
 */
static atomic_t kthread_count = ATOMIC_INIT(0);

/** Entry point for new kernel threads. Make this thread a DDE26
 *  worker and then execute the real thread fn.
 */
static void __kthread_helper(void *arg)
{
	struct __kthread_data k = *((struct __kthread_data *)arg);
	vfree(arg);

	l4dde26_process_add_worker();
	
	do_exit(k.fn(k.arg));
}

/** Our implementation of Linux' kernel_thread() function. Setup a new
 * thread running our __kthread_helper() function.
 */
int kernel_thread(int (*fn)(void *), void *arg, unsigned long flags)
{
    ddekit_thread_t *t;
    char name[20];
	struct __kthread_data *kt = vmalloc(sizeof(struct __kthread_data));

    int threadnum = atomic_inc_return(&kthread_count);
	kt->fn        = fn;
	kt->arg       = arg;

    snprintf(name, 20, ".kthread%x", threadnum);
    t = ddekit_thread_create(__kthread_helper,
	                         (void *)kt, name);
	Assert(t);

	return ddekit_thread_get_id(t);
}

/** Our implementation of exit(). For DDE purposes this only relates
 * to kernel threads.
 */
fastcall void do_exit(long code)
{
	ddekit_thread_t *t = DDEKIT_THREAD(lxtask_to_ddethread(current));
//	printk("Thread %s exits with code %x\n", ddekit_thread_get_name(t), code);

	/* do some cleanup */
	detach_pid(current, 0);
	
	/* goodbye, cruel world... */
	ddekit_thread_exit();
}

/*****************************************************************************
 ** Misc functions                                                          **
 *****************************************************************************/

void dump_stack(void)
{
}

/*****************************************************************************
 ** DDEKit gluecode, init functions                                         **
 *****************************************************************************/
/* Initialize a dde26 thread. 
 *
 * - Allocate thread data, as well as a Linux task struct, 
 * - Fill in default values for thread_info, and task,
 * - Adapt task struct's thread_info backreference
 * - Initialize the DDE sleep lock
 */
static INLINE dde26_thread_data *init_dde26_thread(void)
{
	dde26_thread_data *t = vmalloc(sizeof(dde26_thread_data));
	Assert(t);
	
	memcpy(&LX_THREAD(t), &init_thread, sizeof(struct thread_info));

	LX_TASK(t) = vmalloc(sizeof(struct task_struct));
	Assert(LX_TASK(t));

	memcpy(LX_TASK(t), &init_task, sizeof(struct task_struct));

	/* nice: Linux backreferences a task`s thread_info from the
	*        task struct (which in turn can be found using the
	*        thread_info...) */
	LX_TASK(t)->thread_info = &LX_THREAD(t);

	/* initialize this thread's sleep lock */
	SLEEP_LOCK(t) = ddekit_sem_init(0);

	return t;
}

/* Process setup for worker threads */
int l4dde26_process_add_worker(void)
{
	dde26_thread_data *cur = init_dde26_thread();

	/* If this function is called for a kernel_thread, the thread already has
	 * been set up and we just need to store a reference to the ddekit struct.
	 * However, this function may also be called directly to turn an L4 thread
	 * into a DDE thread. Then, we need to initialize here. */
	cur->_ddekit_thread = ddekit_thread_myself();
	if (cur->_ddekit_thread == NULL)
		cur->_ddekit_thread = ddekit_thread_setup_myself(".dde26_thread");

	ddekit_thread_set_my_data(cur);

	attach_pid(LX_TASK(cur), 0,
               ddekit_thread_get_id(cur->_ddekit_thread));

	return 0;
}

int l4dde26_process_from_ddekit(ddekit_thread_t *t)
{
	dde26_thread_data *cur = init_dde26_thread();
	cur->_ddekit_thread = t;
	ddekit_thread_set_data(t, cur);
	attach_pid(LX_TASK(cur), 0, ddekit_thread_get_id(t));
	return 0;
}

/** Function to initialize the first DDE process.
 */
int l4dde26_process_init(void)
{
	/* init slab caches */
	ddekit_lock_init_unlocked(&_pid_task_list_lock);

	l4dde26_process_add_worker();

	return 0;
}

