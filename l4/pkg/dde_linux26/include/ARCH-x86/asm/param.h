#ifndef _ASMi386_PARAM_H
#define _ASMi386_PARAM_H

#ifdef __KERNEL__
#ifndef DDE_LINUX
# define HZ		1000		/* Internal kernel timer frequency */
#endif /* DDE_LINUX */
# define USER_HZ	100		/* .. some user interfaces are in "ticks" */
# define CLOCKS_PER_SEC	(USER_HZ)	/* like times() */
#endif

#ifndef HZ
#ifndef DDE_LINUX
#define HZ 100
#else /* DDE_LINUX */
/* inside I/O Info Page */
extern unsigned long HZ;
#endif /* DDE_LINUX */
#endif

#define EXEC_PAGESIZE	4096

#ifndef NOGROUP
#define NOGROUP		(-1)
#endif

#define MAXHOSTNAMELEN	64	/* max length of hostname */

#endif
