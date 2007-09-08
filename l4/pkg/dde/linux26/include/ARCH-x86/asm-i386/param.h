#ifndef _ASMi386_PARAM_H
#define _ASMi386_PARAM_H


#ifndef DDE_LINUX
#ifdef __KERNEL__
# define HZ		CONFIG_HZ	/* Internal kernel timer frequency */
# define USER_HZ	100		/* .. some user interfaces are in "ticks" */
# define CLOCKS_PER_SEC		(USER_HZ)	/* like times() */
#endif

#ifndef HZ
#define HZ 100
#endif
#else
extern unsigned long HZ;
#define USER_HZ 	100
#define CLOCKS_PER_SEC (USER_HZ)
#endif /* DDE_LINUX */

#define EXEC_PAGESIZE	4096

#ifndef NOGROUP
#define NOGROUP		(-1)
#endif

#define MAXHOSTNAMELEN	64	/* max length of hostname */
#define COMMAND_LINE_SIZE 256

#endif
