/* $Id$ */

/*	con/server/include/con_macros.h
 *
 *	some macros
 */

#ifndef _CON_MACROS_H
#define _CON_MACROS_H

/* L4 includes */
#include <l4/sys/kdebug.h>

/* local includes */
#include "con_log.h"

/* print error message and panic */
#undef PANIC
#define PANIC(format, args...)                          \
  do {                                                  \
    L4MSG(format"\n", ## args);                         \
    exit(-1);						\
  } while(0)

#endif /* !_CON_MACROS_H */
