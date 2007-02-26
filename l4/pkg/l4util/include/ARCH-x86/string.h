/* 
 * $Id$
 */

#ifndef __L4UTIL_STRING_H
#define __L4UTIL_STRING_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __HAVE_ARCH_STRNCPY
static inline char*
strncpy(char* to, const char* from, int count)
{
  register char *ret = to;

  while ((count-- > 0) && (*to++ = *from++))
    ;
  
  while (count-- > 0)
    *to++ = '\0';

  return ret;
}
#endif

#ifndef __HAVE_ARCH_STRCMP
static inline int
strcmp(const char* s1, const char* s2)
{
  while (*s1 && *s2 && (*s1 == *s2))
    {
      s1++;
      s2++;
    };

  return (*s1 - *s2);
}
#endif

#ifdef __cplusplus
}
#endif

#endif

