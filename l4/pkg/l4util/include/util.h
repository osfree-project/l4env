#ifndef __L4UTIL__UTIL_H__
#define __L4UTIL__UTIL_H__

#include <l4/sys/types.h>
#include <l4/sys/compiler.h>
#include <l4/sys/ipc.h>

EXTERN_C_BEGIN

/** Calculate l4 timeouts
 * \param mus   time in microseconds. Special cases:
 *              - 0 - > timeout 0
 *              - -1 -> timeout NEVER
 * \return the corresponding l4_timeout value
 */
l4_timeout_s l4util_micros2l4to(int mus);

/** Calculate l4 timeouts
 * \param to_m  contains l4-mantissa
 * \param to_e  contains l4-exponent
 * \returns     time in microseconds. Special cases:
 *              - 0 - > timeout 0
 *              - -1 -> timeout NEVER or invalid params
 */
int l4util_l4to2micros(int to_m, int to_e);

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
  for (;;)
    l4_ipc_sleep(L4_IPC_NEVER);
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


#endif /* __L4UTIL__UTIL_H__ */
