/* Mostly copied from Linux. See the DDE_LINUX define for diffs */

#ifndef _ASMi386_PARAM_H
#define _ASMi386_PARAM_H

#ifndef HZ
#ifndef DDE_LINUX
#define HZ 100
#else /* DDE_LINUX */
/* inside I/O Info Page */
extern unsigned long HZ;
#endif /* DDE_LINUX */
#endif

#define EXEC_PAGESIZE	4096

#ifndef NGROUPS
#define NGROUPS		32
#endif

#ifndef NOGROUP
#define NOGROUP		(-1)
#endif

#define MAXHOSTNAMELEN	64	/* max length of hostname */

#ifdef __KERNEL__
# define CLOCKS_PER_SEC	100	/* frequency at which times() counts */
#endif

#endif
