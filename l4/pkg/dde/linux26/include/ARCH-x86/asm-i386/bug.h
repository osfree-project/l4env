#ifndef _I386_BUG_H
#define _I386_BUG_H


/*
 * Tell the user there is some problem.
 * The offending file and line are encoded after the "officially
 * undefined" opcode for parsing in the trap handler.
 */

#ifdef CONFIG_BUG
#define HAVE_ARCH_BUG
#ifdef DDE_LINUX
#include <l4/dde/ddekit/panic.h>
#define BUG() ddekit_panic("\033[31mBUG():\033[0m %s:%d", __FILE__, __LINE__);
#else /* ! DDE_LINUX */

#ifdef CONFIG_DEBUG_BUGVERBOSE
#define BUG()				\
 __asm__ __volatile__(	"ud2\n"		\
			"\t.word %c0\n"	\
			"\t.long %c1\n"	\
			 : : "i" (__LINE__), "i" (__FILE__))
#else
#define BUG() __asm__ __volatile__("ud2\n")
#endif

#endif /* DDE_LINUX */
#endif

#include <asm-generic/bug.h>
#endif
