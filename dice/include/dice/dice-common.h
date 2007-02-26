#ifndef __DICE_COMMON_H__
#define __DICE_COMMON_H__

#ifndef DICE_IID_BITS
#define DICE_IID_BITS 20
#endif

#ifndef DICE_FID_MASK
#define DICE_FID_MASK 0xfffff
#endif

#ifndef min_max_defines
#define min_max_defines
#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)>(y))?(x):(y))
#endif


/*
 * CORBA language mapping types
 */
typedef short              CORBA_short;
typedef long               CORBA_long;
typedef long long          CORBA_long_long;
typedef unsigned short     CORBA_unsigned_short;
typedef unsigned long      CORBA_unsigned_long;
typedef unsigned long long CORBA_unsigned_long_long;
typedef float              CORBA_float;
typedef double             CORBA_double;
typedef long double        CORBA_long_double;
typedef char               CORBA_char;
typedef short              CORBA_wchar;
typedef unsigned char      CORBA_boolean;
typedef int                CORBA_TypeCode;
typedef int                CORBA_int;
typedef struct CORBA_any 
{ 
  CORBA_TypeCode _type; 
  void *_value; 
} CORBA_any;
typedef unsigned long      CORBA_enum;

/*
 * own additions
 */
typedef unsigned char      CORBA_small;
typedef void               CORBA_void;
typedef unsigned char      CORBA_byte;
typedef unsigned char      CORBA_unsigned_char;
typedef void*              CORBA_void_ptr;
typedef char*              CORBA_char_ptr;
typedef const char*        const_CORBA_char_ptr;

/*
 * declares CORBA_alloc and CORBA_free - implementation has to be user provided
 */
void* CORBA_alloc(unsigned long size);
void CORBA_free(void *ptr);

#endif // __DICE_COMMON_H__
