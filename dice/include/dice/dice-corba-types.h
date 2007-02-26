#ifndef __DICE_CORBA_TYPES_H__
#define __DICE_CORBA_TYPES_H__

/* sanity check: test for L4API_* defines. */
#if !defined(L4API_linux) && !defined(L4API_l4v2) && \
    !defined(L4API_l4x0) && !defined(L4API_l4x2) && \
    !defined(L4API_l4v4)
#error Please specify an L4API using -D.
#endif

#ifndef DICE_PTRS_MAX
#define DICE_PTRS_MAX 10
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
/** the Any type of CORBA */
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


#ifndef dice_CORBA_exception_type_typedef
#define dice_CORBA_exception_type_typedef
/** according to CORBA specification:
 *  the exception type
 */
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
#elif defined(L4API_l4x2) || defined(L4API_l4v4)
// KA:
//#include <l4/ipc.h>
//typedef ThreadId CORBA_Object_base;
// DD:
#include <l4/types.h>
typedef L4_ThreadId_t CORBA_Object_base;
#endif

typedef CORBA_Object_base* CORBA_Object;
typedef const CORBA_Object_base * const_CORBA_Object;

#endif // CORBA_Object_typedef

/*
 * some helper macros to handle CORBA_Objects
 */
#ifndef INVALID_CORBA_OBJECT_BASE
#ifdef L4API_linux
#define INVALID_CORBA_OBJECT_BASE_INITIALIZER { AF_UNSPEC, 0, { INADDR_NONE } }
#define INVALID_CORBA_OBJECT_BASE  \
                      ((CORBA_Object_base)INVALID_CORBA_OBJECT_BASE_INITIALIZER)
#elif defined(L4API_l4v2) || defined(L4API_l4x0)
#define INVALID_CORBA_OBJECT_BASE L4_INVALID_ID
#elif defined(L4API_l4x2) || defined(L4API_l4v4)
#define INVALID_CORBA_OBJECT_BASE L4_nilthread
#endif /* L4API_* */
#endif /* !CORBA_OBJECT_BASE_INITIALIZER */

#ifndef CORBA_Environment_typedef
#define CORBA_Environment_typedef

#ifdef L4API_linux

typedef void* (*dice_malloc_func)(size_t);
typedef void (*dice_free_func)(void*);

#include <netinet/in.h>
typedef struct CORBA_Environment
{
  CORBA_exception_type major:4;
  CORBA_exception_type repos_id:28;
  void *param;
      
  in_port_t srv_port;
  int cur_socket;
  void* user_data;
  dice_malloc_func malloc;
  dice_free_func free;
  void* ptrs[DICE_PTRS_MAX];
  unsigned short ptrs_cur;

#ifdef __cplusplus
  CORBA_Environment();
#endif
} CORBA_Environment;

#define CORBA_Server_Environment CORBA_Environment
#elif defined(L4API_l4v2) || defined(L4API_l4x0) 

typedef void* (*dice_malloc_func)(unsigned long);
typedef void (*dice_free_func)(void*);

typedef struct CORBA_Environment
{
  CORBA_exception_type major:4;
  CORBA_exception_type repos_id:28;
  union
    {
      void *param;
      l4_uint32_t ipc_error;
      l4_uint32_t sched_bits;
    } _p;
  
  l4_timeout_t timeout;
  l4_fpage_t rcv_fpage;
  dice_malloc_func malloc;
  dice_free_func free;

#ifdef __cplusplus
  CORBA_Environment();
#endif
} CORBA_Environment;

typedef struct CORBA_Server_Environment
{
  CORBA_exception_type major:4;
  CORBA_exception_type repos_id:28;
  union
    {
      void *param;
      l4_uint32_t ipc_error;
    } _p;
      
  l4_timeout_t timeout;
  l4_fpage_t rcv_fpage;
  dice_malloc_func malloc;
  dice_free_func free;
  
  // server specific
  void* user_data;
  void* ptrs[DICE_PTRS_MAX];
  unsigned short ptrs_cur;

#ifdef __cplusplus
  CORBA_Server_Environment();
#endif
} CORBA_Server_Environment;

#elif defined(L4API_l4x2) || defined(L4API_l4v4)

typedef void* (*dice_malloc_func)(unsigned long);
typedef void (*dice_free_func)(void*);

typedef struct CORBA_Environment
{
  CORBA_exception_type major:4;
  CORBA_exception_type repos_id:28;
  union
    {
      void *param;
      L4_Word32_t ipc_error;
      L4_Word32_t sched_bits;
    } _p;
  
  L4_Time_t timeout;
  L4_Fpage_t rcv_fpage;
  dice_malloc_func malloc;
  dice_free_func free;

#ifdef __cplusplus
  CORBA_Environment();
#endif
} CORBA_Environment;

typedef struct CORBA_Server_Environment
{
  CORBA_exception_type major:4;
  CORBA_exception_type repos_id:28;
  union
    {
      void *param;
      L4_Word32_t ipc_error;
    } _p;
      
  L4_Time_t timeout;
  L4_Fpage_t rcv_fpage;
  dice_malloc_func malloc;
  dice_free_func free;
  
  // server specific
  void* user_data;
  void* ptrs[DICE_PTRS_MAX];
  unsigned short ptrs_cur;

#ifdef __cplusplus
  CORBA_Server_Environment();
#endif
} CORBA_Server_Environment;
#endif

#endif // CORBA_Environment_typedef

/*
 * The helper function section.
 *
 * Declarations first
 */

#ifndef DICE_INLINE
#ifdef L4_INLINE
#define DICE_INLINE L4_INLINE
#else
#define DICE_INLINE static inline
#endif /* L4_INLINE */
#endif /* !DICE_INLINE */

/**
 * \brief test if two CORBA_Objects are equal
 * \ingroup dice_types
 *
 * \param o1     first CORBA_Object
 * \param o2     second CORBA_Object
 *
 * \return 1 if they are equal
 */
DICE_INLINE int 
dice_is_obj_equal(CORBA_Object o1, CORBA_Object o2);


#if defined(L4API_l4v2) || defined(L4API_l4x0)

/**
 * \brief set scheduling parameters in the environment
 * \ingroup dice_types
 *
 * \param env        environment to manipulate
 * \param sched_mask the bit mask of scheduling parameters
 *                   -# L4_IPC_DECEIT_MASK
 *
 * Replaces the values of the scheduling bitmask in the
 * environment with the given value. There is no `unset'
 * function, simple remove the bit from the mask.
 */
DICE_INLINE void
dice_l4_sched_set(CORBA_Environment *env, l4_uint32_t sched_mask);

#elif defined(L4API_l4v2) || defined(L4API_l4x0)

/**
 * \brief set scheduling parameters in the environment
 * \ingroup dice_types
 *
 * \param env        environment to manipulate
 * \param sched_mask the bit mask of scheduling parameters
 *                   -# L4_IPC_DECEIT_MASK
 *
 * Replaces the values of the scheduling bitmask in the
 * environment with the given value. There is no `unset'
 * function, simple remove the bit from the mask.
 */
DICE_INLINE void
dice_l4_sched_set(CORBA_Environment *env, L4_Word32_t sched_mask);

#endif

#ifdef L4API_linux

DICE_INLINE int
dice_is_obj_equal(CORBA_Object o1, CORBA_Object o2)
{
  if (!o1 || !o2)
    return 0;
  return ((o1->sin_family      == o2->sin_family) &&
          (o1->sin_port        == o2->sin_port) &&
	  (o1->sin_addr.s_addr == o2->sin_addr.s_addr));
}

#elif defined(L4API_l4v2) || defined(L4API_l4x0)

DICE_INLINE int
dice_is_obj_equal(CORBA_Object o1, CORBA_Object o2)
{
    if (!o1 || !o2)
	return 0;
    return l4_thread_equal(*o1, *o2);
}

DICE_INLINE void
dice_l4_sched_set(CORBA_Environment *env, l4_uint32_t bit_mask)
{
  env->_p.sched_bits = bit_mask;
}

#elif defined(L4API_l4x2) || defined(L4API_l4v4)

DICE_INLINE int
dice_is_obj_equal(CORBA_Object o1, CORBA_Object o2)
{
    if (!o1 || !o2)
	return 0;
    return L4_IsThreadEqual(*o1, *o2);
}

DICE_INLINE void
dice_l4_sched_set(CORBA_Environment *env, L4_Word32_t bit_mask)
{
  env->_p.sched_bits = bit_mask;
}

#endif /* L4API_* */

#endif // __DICE_CORBA_TYPES_H__

