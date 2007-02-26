#ifndef __DICE_DICE_L4_TESTSUITE_H__
#define __DICE_DICE_L4_TESTSUITE_H__


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
  return 123;
}

// needed for debug output via printf
#warning should use <l4io.h>
//#include <l4io.h>
#include <stdio.h>

#endif /* L4API */


/*
 * define random function to be used with test-suite
 */
#define random_CORBA_short              (CORBA_short)dice_rand()
#define random_CORBA_long               (CORBA_long)dice_rand()
#define random_CORBA_long_long          (CORBA_long_long)dice_rand()
#define random_CORBA_unsigned_short     (CORBA_unsigned_short)dice_rand()
#define random_CORBA_unsigned_long      (CORBA_unsigned_long)dice_rand()
#define random_CORBA_unsigned_long_long (CORBA_unsigned_long_long)dice_rand()
#define random_CORBA_float              (CORBA_float)(dice_rand()/(double)dice_rand())
#define random_CORBA_double             (CORBA_double)(dice_rand()/(double)dice_rand())
#define random_CORBA_long_double        (CORBA_long_double)(dice_rand()/(double)dice_rand())
#define random_CORBA_char               (CORBA_char)dice_rand()
#define random_CORBA_unsigned_char      (CORBA_unsigned_char)dice_rand()
#define random_CORBA_wchar              (CORBA_wchar)dice_rand()
#define random_CORBA_boolean            (CORBA_boolean)dice_rand()
#define random_CORBA_TypeCode           (CORBA_TypeCode)dice_rand()
#define random_CORBA_enum               (CORBA_enum)dice_rand()
#define random_CORBA_small              (CORBA_small)dice_rand()
#define random_CORBA_byte               (CORBA_byte)dice_rand()
#define random_CORBA_int                (CORBA_int)dice_rand()
#define random_CORBA_unsigned_int       (CORBA_unsigned_int)dice_rand()

#define random_CORBA_char_ptr  (const CORBA_char_ptr)"Hello World! This is a test string.\n"

#define random_l4_int16_t               (l4_int16_t)dice_rand()
#define random_l4_int32_t               (l4_int32_t)dice_rand()
#define random_l4_int64_t               (l4_int64_t)dice_rand()
#define random_l4_uint16_t              (l4_uint16_t)dice_rand()
#define random_l4_uint32_t              (l4_uint32_t)dice_rand()
#define random_l4_uint64_t              (l4_uint64_t)dice_rand()
#define random_float                    (float)(dice_rand()/(double)dice_rand())
#define random_double                   (double)(dice_rand()/(double)dice_rand())
#define random_long_double              (long double)(dice_rand()/(double)dice_rand())
#define random_int                      (int)dice_rand()
#define random_short                    (short)dice_rand()
#define random_long                     (long)dice_rand()

#if defined(L4API_l4v2) || defined(L4API_l4x0)
#include "dice/dice-l4-v2x0-testsuite.h"
#elif defined(L4API_l4x2) || defined(L4API_l4v4)
#include "dice/dice-l4-v4-testsuite.h"
#endif /* L4API */

#endif /* __DICE_DICE_L4_TESTSUITE_H__ */

