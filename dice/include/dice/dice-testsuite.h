#ifndef __DICE_DICE_TESTSUITE_H__
#define __DICE_DICE_TESTSUITE_H__

#if defined(L4API_l4v2) || defined(L4API_l4x0)

// needed for random functions
#include <l4/util/rand.h>
// needed fot reboot
#include <l4/util/reboot.h>
// needed for debug output via printf
#include <stdio.h>

/* declare random function */
L4_INLINE
unsigned long
dice_rand(void);
/* define random function */
L4_INLINE
unsigned long 
dice_rand(void)
{
  return l4util_rand();
}

#elif defined(L4API_l4x2) || defined(L4API_l4v4)

#include <l4/types.h>

/* declare random function */
L4_INLINE
unsigned long
dice_rand(void);
/* define random function */
L4_INLINE
unsigned long
dice_rand(void)
{
  return 0x20051976;
}

// needed for debug output via printf
#include <l4io.h>

#elif defined(L4API_linux)

#include <stdlib.h>

/* declare random function */
inline
unsigned long
dice_rand(void);
/* define random function */
inline
unsigned long
dice_rand(void)
{
    return (unsigned long)random();
}

#endif /* L4API_* */


/*
 * define random function to be used with test-suite
 */
#define random_float                    (float)((float)dice_rand() + (float)dice_rand()/(double)dice_rand())
#define random_double                   (double)((double)dice_rand() + (double)dice_rand()/(double)dice_rand())
#define random_long_double              (long double)((long double)dice_rand() + (double)dice_rand()/(double)dice_rand())
#define random_int                      (int)dice_rand()
#define random_unsigned_int             (unsigned int)dice_rand()
#define random_short                    (short)dice_rand()
#define random_long                     (long)dice_rand()
#define random_unsigned_long            (unsigned long)dice_rand()
#define random_long_long                ((long long)dice_rand() << 32) | (long long)dice_rand()
#define random_unsigned_long_long       (unsigned long long)(((long long)dice_rand() << 32) | (long long)dice_rand())
#define random_char                     (char)dice_rand()
#define random_unsigned_char            (unsigned char)dice_rand()

#if defined(L4API_l4v2) || defined(L4API_l4x0)
#include "dice/dice-l4-v2x0-testsuite.h"
#elif defined(L4API_l4x2) || defined(L4API_l4v4)
#include "dice/dice-l4-v4-testsuite.h"
#endif /* L4API */

#endif /* __DICE_DICE_TESTSUITE_H__ */
