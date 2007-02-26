/* $Id$ */

#ifndef _LINUX_INIT_H
#define _LINUX_INIT_H

#include <linux/config.h>

/* These macros are used to mark some functions or 
 * initialized data (doesn't apply to uninitialized data)
 * as `initialization' functions. The kernel can take this
 * as hint that the function is used only during the initialization
 * phase and free up used memory resources after
 *
 * Usage:
 * For functions:
 * 
 * You should add __init immediately before the function name, like:
 *
 * static void __init initme(int x, int y)
 * {
 *    extern int z; z = x * y;
 * }
 *
 * If the function has a prototype somewhere, you can also add
 * __init between closing brace of the prototype and semicolon:
 *
 * extern int initialize_foobar_device(int, int, int) __init;
 *
 * For initialized data:
 * You should insert __initdata between the variable name and equal
 * sign followed by value, e.g.:
 *
 * static int init_variable __initdata = 0;
 * static char linux_logo[] __initdata = { 0x32, 0x36, ... };
 *
 * Don't forget to initialize data not at file scope, i.e. within a function,
 * as gcc otherwise puts the data into the bss section and not into the init
 * section.
 */

#ifndef MODULE

#ifndef __ASSEMBLY__

/*
 * Used for initialization calls..
 */
typedef int (*initcall_t)(void);
typedef void (*exitcall_t)(void);

extern initcall_t __initcall_start, __initcall_end;

#define __initcall(fn)	\
	int initcall_##fn(void)	\
	{ return fn(); }
#define __exitcall(fn)	\
	int exitcall_##fn(void)	\
	{ return fn(); }

/*
 * Used for kernel command line parameter setup
 */
struct kernel_param {
	const char *str;
	int (*setup_func)(char *);
};

extern struct kernel_param __setup_start, __setup_end;

#define __setup(str, fn)								\
	static char __setup_str_##fn[] __initdata = str;				\
	static struct kernel_param __setup_##fn __attribute__((unused)) __initsetup = { __setup_str_##fn, fn }

#endif /* __ASSEMBLY__ */

/*
 * Mark functions and data as being only used at initialization
 * or exit time.
 */
#define __init
#define __exit
#define __initdata
#define __exitdata
#define __initsetup
#define __init_call
#define __exit_call

/* For assembly routines */
#define __INIT
#define __FINIT
#define __INITDATA

#define module_init(x)	__initcall(x);
#define module_exit(x)	__exitcall(x);

#else
#error defined(MODULE)
#endif /* !MODULE */

#define __devinit
#define __devinitdata
#define __devexit
#define __devexitdata

#endif /* _LINUX_INIT_H */
