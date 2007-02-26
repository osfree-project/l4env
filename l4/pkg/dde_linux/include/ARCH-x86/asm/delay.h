/* Mostly copied from Linux. See the DDE_LINUX define for diffs */

#ifndef _I386_DELAY_H
#define _I386_DELAY_H

/*
 * Copyright (C) 1993 Linus Torvalds
 *
 * Delay routines calling functions in arch/i386/lib/delay.c
 */
 
extern void __bad_udelay(void);

extern void __udelay(unsigned long usecs);
extern void __const_udelay(unsigned long usecs);
extern void __delay(unsigned long loops);

#ifndef DDE_LINUX
#define udelay(n) (__builtin_constant_p(n) ? \
	((n) > 20000 ? __bad_udelay() : __const_udelay((n) * 0x10c6ul)) : \
	__udelay(n))
#else /* DDE_LINUX */
extern void udelay(unsigned long usecs);
#endif /* DDE_LINUX */

#endif /* defined(_I386_DELAY_H) */
