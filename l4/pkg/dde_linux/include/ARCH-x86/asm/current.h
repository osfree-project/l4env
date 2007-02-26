/* Mostly copied from Linux. See the DDE_LINUX define for diffs */

#ifndef _I386_CURRENT_H
#define _I386_CURRENT_H

struct task_struct;

#ifndef DDE_LINUX
static inline struct task_struct * get_current(void)
{
	struct task_struct *current;
	__asm__("andl %%esp,%0; ":"=r" (current) : "0" (~8191UL));
	return current;
 }
#else /* DDE_LINUX */
extern struct task_struct * get_current(void);
#endif /* DDE_LINUX */
 
#define current get_current()

#endif /* !(_I386_CURRENT_H) */
