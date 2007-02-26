#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdlib.h>
#include <flux/machine/types.h>
#include <l4/sys/types.h>

/* globals defined here (when included from globals.c) */

#ifdef DEFINE_GLOBALS
# define __EXTERN
#else
# define __EXTERN extern
#endif

/* thread ids of global importance */
__EXTERN l4_threadid_t my_pager, my_preempter, myself;

/* number of small address spaces configured at boot */
__EXTERN unsigned small_space_size;

__EXTERN int debug;
__EXTERN int jochen;		/* be bug-compatible with Jochen's L4 */

#undef __EXTERN

/* more global stuff */

typedef l4_uint8_t owner_t;		/* an owner is a task number < 256 */
#define O_MAX (255)
#define O_FREE (0)
#define O_RESERVED (1)

#define TASK_MAX (1L << 11)

extern int __crt_dummy__;	/* begin of RMGR memory -- defined in crt0.S */
extern int _stext;		/* begin of RGMR text -- defined by rmgr.ld */
extern int _stack;		/* end of RMGR stack -- defined in crt0.S */
extern int _end;		/* end of RGMR memory -- defined by rmgr.ld */

/* the check macro is like assert(), but also evaluates its argument
   if NCHECK is defined */
#ifdef NCHECK
# define check(expression) ((void)(expression))
#else /* ! NCHECK */
# define check(expression) \
         ((void)((expression) ? 0 : \
                (panic(__FILE__":%u: failed check `"#expression"'", \
                        __LINE__), 0)))
#endif /* ! NCHECK */

#endif
