#ifndef UTIL_H__
#define UTIL_H__

#include <l4/sys/types.h>
#include <l4/sys/compiler.h>
#include <l4/sys/ipc.h>

EXTERN_C_BEGIN

/** Calculate l4 timeouts */
int micros2l4to(int mus, int *to_e, int *to_m);

/** Suspend thread for a period of <ms> milliseconds */
void l4_sleep(int ms);

/* Suspend thread for a period of <us> micro seconds.
 * WARNING: This function is mostly bogus since the timer resolution of
 *          current L4 implementations is about 1ms! */
void l4_usleep(int us);

/** Go sleep and never wake up. */
L4_INLINE void l4_sleep_forever(void) __attribute__((noreturn));

L4_INLINE void
l4_sleep_forever(void)
{
  l4_msgdope_t result;
  l4_umword_t dummy;
  l4_ipc_receive(L4_NIL_ID, L4_IPC_SHORT_MSG, &dummy, &dummy,
                 L4_IPC_NEVER, &result);
  while(1);
}

/** Touch data areas to force mapping read-only */
static inline void
l4_touch_ro(const void*addr, unsigned size)
{ 
  volatile const char *bptr, *eptr;

  bptr = (const char*)(((unsigned)addr) & L4_PAGEMASK);
  eptr = (const char*)(((unsigned)addr+size-1) & L4_PAGEMASK);
  for(;bptr<=eptr;bptr+=L4_PAGESIZE) {
    (void)(*bptr);
  }
}


/** Touch data areas to force mapping read-write */
static inline void
l4_touch_rw(const void*addr, unsigned size)
{
  volatile char *bptr;
  volatile const char *eptr;
      
  bptr = (char*)(((unsigned)addr) & L4_PAGEMASK);
  eptr = (const char*)(((unsigned)addr+size-1) & L4_PAGEMASK);
  for(;bptr<=eptr;bptr+=L4_PAGESIZE) {
    char x = *bptr; 
    *bptr = x;
  }
}

EXTERN_C_END


#endif /* UTIL_H__ */
