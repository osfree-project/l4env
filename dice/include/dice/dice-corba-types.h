#ifndef __DICE_CORBA_TYPES_H__
#define __DICE_CORBA_TYPES_H__

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
typedef struct CORBA_any 
{ 
  CORBA_TypeCode _type; 
  void *_value; 
} CORBA_any;
typedef unsigned long      CORBA_enum;
typedef unsigned int       CORBA_exception_type;

/*
 * own additions
 */
typedef unsigned char      CORBA_small;
typedef void               CORBA_void;
typedef unsigned char      CORBA_byte;
typedef unsigned char      CORBA_unsigned_char;
typedef int                CORBA_int;
typedef unsigned int       CORBA_unsigned_int;
typedef void*              CORBA_void_ptr;
typedef const void*        const_CORBA_void_ptr;
typedef char*              CORBA_char_ptr;
typedef const char*        const_CORBA_char_ptr;


#define DICE_COMMON_ENVIRONMENT \
  CORBA_exception_type major:4; \
  CORBA_exception_type repos_id:28; \
  void *param

#ifndef dice_CORBA_exception_type_typedef
#define dice_CORBA_exception_type_typedef
typedef union
{
  struct
    {
      int major:4;
      int repos_id:28;
    } _s;
  unsigned _raw;
} dice_CORBA_exception_type;
#endif // dice_CORBA_exception_type_typedef

#ifndef CORBA_Object_typedef
#define CORBA_Object_typedef

#ifdef L4API_linux
typedef struct sockaddr_in CORBA_Object_base;
#elif defined(L4API_l4v2) || defined(L4API_l4x0)
#include <l4/sys/types.h>
typedef l4_threadid_t CORBA_Object_base;
#endif 

typedef CORBA_Object_base* CORBA_Object;

#endif // CORBA_Object_typedef

#ifndef CORBA_Environment_typedef
#define CORBA_Environment_typedef

#ifdef L4API_linux

typedef void* (*dice_malloc_func)(size_t);
typedef void (*dice_free_func)(void*);

#include <netinet/in.h>
typedef struct CORBA_Environment
{
  DICE_COMMON_ENVIRONMENT;
  in_port_t srv_port;
  int cur_socket;
  void* user_data;
  dice_malloc_func malloc;
  dice_free_func free;
} CORBA_Environment;
#elif defined(L4API_l4v2) || defined(L4API_l4x0)

typedef void* (*dice_malloc_func)(unsigned long);
typedef void (*dice_free_func)(void*);

typedef struct CORBA_Environment
{
  DICE_COMMON_ENVIRONMENT;
  l4_timeout_t timeout;
  l4_fpage_t rcv_fpage;
  l4_uint32_t ipc_error;
  void* user_data;
  dice_malloc_func malloc;
  dice_free_func free;
} CORBA_Environment;
#endif

#endif // CORBA_Environment_typedef

#endif // __DICE_CORBA_TYPES_H__

