#ifndef CRTX_CTOR_H
#define CRTX_CTOR_H

typedef void (*l4ctor_t)(void);

#define l4str(s) #s

#define L4C_CTOR(fn, prio)                                               \
  static l4ctor_t __l4ctor_##fn                                          \
  __attribute__ ((unused, __section__(".c_ctors." l4str(prio)))) = fn \

#define L4C_DTOR(fn, prio)                                               \
  static l4ctor_t __l4ctor_##fn                                          \
  __attribute__ ((unused, __section__(".c_dtors." l4str(prio)))) = fn \

/** Constructors to be executed before the backend is initialized. Typical
 *  example is registering the IPC error codes allowing verbose messages
 *  in the backend initialization process. */
#define L4CTOR_BEFORE_BACKEND			1000

/** Constructors reserved for initializing of the L4env/libc backend. */
#define L4CTOR_BACKEND				2000

/** Constructors to be initialized after the L4env/libc backend is up. */
#define L4CTOR_AFTER_BACKEND			3000


#endif

