/******************************************************************************
 * L4Linux kernel module showing the use of task server callbacks to          *
 * implement a task cache that can seriously speed up L4Linux' fork()         *
 * performance.                                                               *
 *                                                                            *
 * Before using this, perform the following steps:                            *
 * 1) Adapt the Makefile, so that the kernel directory points to your         *
 *    private L4Linux kernel and build directories.                           *
 * 2) Make sure that your version of L4Linux exports the following functions: *
 *    - l4ts_allocate_task()                                                  *
 *    - l4ts_free_task()                                                      *
 *    - l4task_client_register_hooks()                                        *
 *                                                                            *
 * You can then use the generated kernel module and insmod it.                *
 *                                                                            *
 * Bjoern Doebel <doebel@tudos.org>                                           *
 *                                                                            *
 * (c) 2005 - 2007 Technische Universitaet Dresden                            *
 * This file is part of DROPS, which is distributed under the terms of the    *
 * GNU General Public License 2. Please see the COPYING file for details.     *
 ******************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>	/* Needed for KERN_INFO */

#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/task/task_client.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("L4Linux task cache.");
MODULE_AUTHOR("Bjoern Doebel <doebel@tudos.org>");

#define TASK_CACHE_SIZE	5

#define DEBUG_MSG(msg, ...)	/*LOG_printf("%s:\033[34m"msg"\033[0m", __FUNCTION__, ##__VA_ARGS__)*/

long l4ts_alloc_cb(l4_taskid_t *task, char *status);
long l4ts_free_cb(const l4_taskid_t *task, char *status);

l4task_client_callbacks task_cache_cb = {
	.client_alloc_callback = l4ts_alloc_cb,
	.client_free_callback = l4ts_free_cb,
	.client_exit_callback = NULL,
	.client_create_callback = NULL,
	.client_kill_callback = NULL
};

struct
{
	char        taken;
	l4_taskid_t task;
} task_cache[TASK_CACHE_SIZE];


long l4ts_alloc_cb(l4_taskid_t *task, char *status)
{
	int i;
	for (i=0; i < TASK_CACHE_SIZE; ++i) {
		if (task_cache[i].taken == 0) {
			DEBUG_MSG("taking task "l4util_idfmt" from task cache.\n",
					  l4util_idstr(task_cache[i].task));
			*task = task_cache[i].task;
			task_cache[i].taken = 1;
			*status = CALLBACK_SKIP;
			break;
		}
	}
	return 0;
}


long l4ts_free_cb(const l4_taskid_t *task, char *status)
{
	int i;

	for (i=0; i < TASK_CACHE_SIZE; ++i) {
		if (task_cache[i].taken && l4_task_equal(task_cache[i].task, *task)) {
			DEBUG_MSG("returning task "l4util_idfmt" to task cache\n",
					  l4util_idstr(*task));
			task_cache[i].taken = 0;
			*status = CALLBACK_SKIP;
			break;
		}
	}
	return 0;
}


int __init setup(void)
{
	int i;
	DEBUG_MSG("Hello...\n");

	for (i = 0; i < TASK_CACHE_SIZE; ++i) {
		int r = l4ts_allocate_task(&task_cache[i].task);
		if (r) {
			int j;
			DEBUG_MSG("Cannot allocate task for task_cache.\n");
			/* return all previously allocated tasks. */
			for (j=i-1; j >= 0; --j)
				l4ts_free_task(&task_cache[j].task);
			return -1;
		}
		task_cache[i].taken = 0;
	}

	l4task_client_register_hooks(task_cache_cb);
	DEBUG_MSG("registered tasklib callbacks.\n");

	return 0;
}

void __exit teardown(void)
{
	int i;
	l4task_client_callbacks null_cb = {
		.client_alloc_callback = NULL,
		.client_free_callback = NULL,
		.client_exit_callback = NULL,
		.client_create_callback = NULL,
		.client_kill_callback = NULL
	};
	l4task_client_register_hooks(null_cb);

	for (i=0; i<TASK_CACHE_SIZE; i++) {
		l4ts_free_task(&task_cache[i].task);
		task_cache[i].taken = 0;
	}
}

module_init(setup);
module_exit(teardown);
