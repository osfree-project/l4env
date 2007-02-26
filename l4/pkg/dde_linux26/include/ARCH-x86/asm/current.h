#ifndef _I386_CURRENT_H
#define _I386_CURRENT_H

#include <linux/thread_info.h>

struct task_struct;

#ifndef DDE_LINUX
static inline struct task_struct * get_current(void)
{
	return current_thread_info()->task;
}
#else /* DDE_LINUX */
extern struct task_struct * get_current(void);
#endif /* DDE_LINUX */
 
#define current get_current()

#endif /* !(_I386_CURRENT_H) */
