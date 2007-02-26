/* 
 * $Id$
 */

#ifndef __L4_STDIO_H
#define __L4_STDIO_H

#include <stdarg.h>
#include <l4/sys/types.h>
#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

/* io oriented stuff */

int l4util_sprintf(char *buf, const char *cfmt, ...);
int l4util_vsprintf(char* string, __const char* format, va_list);


EXTERN_C_END

#endif

