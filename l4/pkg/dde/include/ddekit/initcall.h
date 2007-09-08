#ifndef _ddekit_initcall_h
#define _ddekit_initcall_h

// from l4/sys/compiler.h
#if (__GNUC__ == 3 && __GNUC_MINOR__ >= 3) || __GNUC__ >= 4
#define L4_STICKY(x)    __attribute__((used)) x
#else
#define L4_STICKY(x)    __attribute__((unused)) x
#endif

#define l4str(s) #s

// from dde_linux/ARCH-x86/ctor.h
typedef void (*l4ddekit_initcall_t)(void);

/** Define a function to be a DDEKit initcall. 
 *
 *  Define a function to be a DDEKit initcall. This function will then be placed
 *  in a separate linker section of the binary (called .l4dde_ctors). The L4Env
 *  construction mechanism will execute all constructors in this section during
 *  application startup.
 *
 *  This is the right place to place Linux' module_init functions & Co.
 *
 *  \param fn    function
 */
#define DDEKIT_INITCALL(fn)	\
	static l4ddekit_initcall_t \
	L4_STICKY(__l4ddekit_initcall_##fn) __l4ddekit_init_call(1) = (void *)fn

#if 0
#define __l4ddekit_init_call 	\
	__attribute__ ((__section__ (".l4dde_ctors")))
#endif

#define __l4ddekit_initcall(p) \
	__attribute__ ((__section__ (".l4dde_ctors." l4str(p))))

#define DDEKIT_CTOR(fn, prio) \
	static l4ddekit_initcall_t \
	L4_STICKY(__l4ddekit_initcall_##fn) \
	__l4ddekit_initcall(prio) = (void *)fn

/**
 * Runs all registered initcalls.
 */
void ddekit_do_initcalls(void);

#endif
