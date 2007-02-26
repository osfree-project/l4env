#ifndef __DICE_L4_COMMON_H__
#define __DICE_L4_COMMON_H__

#include "dice/dice-common.h"

// needed for random
#include <l4/util/rand.h>

/*
 * define random function to be used with test-suite
 */
#define random_CORBA_short              (CORBA_short)l4_rand()
#define random_CORBA_long               (CORBA_long)l4_rand()
#define random_CORBA_long_long          (CORBA_long_long)l4_rand()
#define random_CORBA_unsigned_short     (CORBA_unsigned_short)l4_rand()
#define random_CORBA_unsigned_long      (CORBA_unsigned_long)l4_rand()
#define random_CORBA_unsigned_long_long (CORBA_unsigned_long_long)l4_rand()
#define random_CORBA_float              (CORBA_float)(l4_rand()/(double)l4_rand())
#define random_CORBA_double             (CORBA_double)(l4_rand()/(double)l4_rand())
#define random_CORBA_long_double        (CORBA_long_double)(l4_rand()/(double)l4_rand())
#define random_CORBA_char               (CORBA_char)l4_rand()
#define random_CORBA_unsigned_char      (CORBA_unsigned_char)l4_rand()
#define random_CORBA_wchar              (CORBA_wchar)l4_rand()
#define random_CORBA_boolean            (CORBA_boolean)l4_rand()
#define random_CORBA_TypeCode           (CORBA_TypeCode)l4_rand()
#define random_CORBA_enum               (CORBA_enum)l4_rand()
#define random_CORBA_small              (CORBA_small)l4_rand()
#define random_CORBA_byte               (CORBA_byte)l4_rand()

#endif
