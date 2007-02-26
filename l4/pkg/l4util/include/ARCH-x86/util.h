/* 
 * $Id$
 */

#ifndef __UTIL_H
#define __UTIL_H

#include <stdarg.h>
#include <l4/sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* io oriented stuff */

int 
l4_sprintf(char *buf, const char *cfmt, ...);
int 
l4_vsprintf(char* string, __const char* format, va_list);

/* calculate l4 timeouts */
int micros2l4to(int mus, int *to_e, int *to_m);

/* suspend thread */
void l4_sleep(int ms);
void l4_usleep(int us);


/* touching data areas to force mapping */
static inline void
l4_touch_ro(const void*addr, unsigned size)
{
  const char *bptr, *eptr;

  bptr = (const char*)(((unsigned)addr) & L4_PAGEMASK);
  eptr = (const char*)(((unsigned)addr+size-1) & L4_PAGEMASK);
  for(;bptr<=eptr;bptr+=L4_PAGESIZE){
    asm volatile("or	%0,%%eax \n"
                 :
                 : "m" (*(const unsigned*)bptr)
                 : "eax" );
  }
}

static inline void
l4_touch_rw(const void*addr, unsigned size)
{
  const char *bptr, *eptr;
      
  bptr = (const char*)(((unsigned)addr) & L4_PAGEMASK);
  eptr = (const char*)(((unsigned)addr+size-1) & L4_PAGEMASK);
  for(;bptr<=eptr;bptr+=L4_PAGESIZE){
    asm volatile("or	$0,%0 \n"
                 :
                 : "m" (*(const unsigned*)bptr)
                 );
  }
}

#ifdef __cplusplus
}
#endif

#endif
