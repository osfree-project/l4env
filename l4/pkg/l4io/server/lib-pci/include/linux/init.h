/* include original header */
#include_next <linux/init.h>

#ifndef MODULE

#undef __initcall
#undef __exitcall

#define __initcall(fn)	\
  int initcall_##fn(void) { return fn(); }
#define __exitcall(fn)	\
  int exitcall_##fn(void) { return fn(); }

#undef __init
#undef __exit
#undef __initdata
#undef __exitdata
#undef __initsetup
#undef __init_call
#undef __exit_call
#undef __INIT
#undef __FINIT
#undef __INITDATA

#define __init
#define __exit
#define __initdata
#define __exitdata
#define __initsetup
#define __init_call
#define __exit_call
#define __INIT
#define __FINIT
#define __INITDATA

#else
# error defined(MODULE)
#endif
