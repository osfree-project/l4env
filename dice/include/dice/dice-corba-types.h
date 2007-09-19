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

#ifdef __cplusplus
#define NAMESPACE_DICE_BEG namespace dice {
#define NAMESPACE_DICE_END }
#else
#define NAMESPACE_DICE_BEG
#define NAMESPACE_DICE_END
#endif

NAMESPACE_DICE_BEG

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
      CORBA_exception_type major:4;
      CORBA_exception_type repos_id:28;
    } _corba;
  unsigned _raw;
} dice_CORBA_exception_type;
#endif // dice_CORBA_exception_type_typedef

/* close dice namespace before includes */
NAMESPACE_DICE_END

#ifndef CORBA_Object_typedef
#define CORBA_Object_typedef

#ifdef L4API_linux

#include <sys/types.h>

NAMESPACE_DICE_BEG

typedef struct sockaddr_in CORBA_Object_base;

NAMESPACE_DICE_END

#elif defined(L4API_l4v2) || defined(L4API_l4x0)

#include <l4/sys/types.h>

NAMESPACE_DICE_BEG

typedef l4_threadid_t CORBA_Object_base;

NAMESPACE_DICE_END

#elif defined(L4API_l4x2) || defined(L4API_l4v4)
// KA:
//#include <l4/ipc.h>
//typedef ThreadId CORBA_Object_base;
// DD:
#include <l4/types.h>
#include <l4/message.h>

NAMESPACE_DICE_BEG

typedef L4_ThreadId_t CORBA_Object_base;

NAMESPACE_DICE_END

#endif /* L4API_ */

NAMESPACE_DICE_BEG

typedef CORBA_Object_base* CORBA_Object;
typedef const CORBA_Object_base * const_CORBA_Object;

NAMESPACE_DICE_END

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

NAMESPACE_DICE_BEG

typedef void* (*dice_malloc_func)(size_t);
typedef void (*dice_free_func)(void*);

NAMESPACE_DICE_END

#include <netinet/in.h>
#include <sys/time.h>

NAMESPACE_DICE_BEG

typedef struct CORBA_Environment
{
    dice_CORBA_exception_type _exception;
    void *param;
    
    in_port_t srv_port;
    int cur_socket;
    void* user_data;
    dice_malloc_func malloc;
    dice_free_func free;

    CORBA_Object_base partner;
    void* ptrs[DICE_PTRS_MAX];
    unsigned short ptrs_cur;

    struct timeval receive_timeout;
    
#ifdef __cplusplus
    CORBA_Environment();
    // effective C++ warnings
private:
    CORBA_Environment(const CORBA_Environment &);
    CORBA_Environment& operator=(const CORBA_Environment &);
#endif
} CORBA_Environment;

NAMESPACE_DICE_END

#define CORBA_Server_Environment CORBA_Environment
#elif defined(L4API_l4v2) || defined(L4API_l4x0) 

NAMESPACE_DICE_BEG

typedef void* (*dice_malloc_func)(unsigned long);
typedef void (*dice_free_func)(void*);

typedef struct CORBA_Environment
{
    dice_CORBA_exception_type _exception;
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
    // effective C++ warnings
private:
    CORBA_Environment(const CORBA_Environment &);
    CORBA_Environment& operator=(const CORBA_Environment &);
#endif
} CORBA_Environment;

typedef struct CORBA_Server_Environment
{
    dice_CORBA_exception_type _exception;
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
  
    // server specific
    CORBA_Object_base partner;
    void* user_data;
    void* ptrs[DICE_PTRS_MAX];
    unsigned short ptrs_cur;

#ifdef __cplusplus
    CORBA_Server_Environment();
    // effective C++ warnings
private:
    CORBA_Server_Environment(const CORBA_Server_Environment &);
    CORBA_Server_Environment& operator=(const CORBA_Server_Environment &);
#endif
} CORBA_Server_Environment;

NAMESPACE_DICE_END

#elif defined(L4API_l4x2) || defined(L4API_l4v4)

NAMESPACE_DICE_BEG

typedef void* (*dice_malloc_func)(unsigned long);
typedef void (*dice_free_func)(void*);

typedef struct CORBA_Environment
{
    dice_CORBA_exception_type _exception;
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
    // effective C++ warnings
private:
    CORBA_Environment(const CORBA_Environment &);
    CORBA_Environment& operator=(const CORBA_Environment &);
#endif
} CORBA_Environment;

typedef struct CORBA_Server_Environment
{
    dice_CORBA_exception_type _exception;
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
    
    // server specific
    CORBA_Object_base partner;
    void* user_data;
    void* ptrs[DICE_PTRS_MAX];
    unsigned short ptrs_cur;
    
#ifdef __cplusplus
    CORBA_Server_Environment();
    // effective C++ warnings
private:
    CORBA_Server_Environment(const CORBA_Server_Environment &);
    CORBA_Server_Environment& operator=(const CORBA_Server_Environment &);
#endif
} CORBA_Server_Environment;

NAMESPACE_DICE_END

#endif /* x2 || v4 */

/** \def access exception major member */
#define DICE_EXCEPTION_MAJOR(env) (env)->_exception._corba.major

/** \def access exception minor (repos_id) member */
#define DICE_EXCEPTION_MINOR(env) (env)->_exception._corba.repos_id

#if defined(L4API_l4v2) || defined(L4API_l4x0) || defined(L4API_l4x2) || defined(L4API_l4v4)

/** \def access exception param member */
#define DICE_EXCEPTION_PARAM(env) (env)->_p.param

/** \def access exception ipc error member */
#define DICE_IPC_ERROR(env) (env)->_p.ipc_error

#else 

/** \def access exception param member */
#define DICE_EXCEPTION_PARAM(env) (env)->param

/** \def access exception ipc error member */
#define DICE_IPC_ERROR(env) DICE_EXCEPTION_MINOR(env)

#endif // l4v2 || l4x0 || l4x2 || l4v4

/** \def check if environment contains exception */
#define DICE_IS_EXCEPTION(env, exc) \
    (DICE_EXCEPTION_MAJOR(env) == exc)

/** \def check if there is exception at all */
#define DICE_HAS_EXCEPTION(env) \
    (DICE_EXCEPTION_MAJOR(env) != CORBA_NO_EXCEPTION)
    
/** \def check if no exception occured */
#define DICE_IS_NO_EXCEPTION(env) \
    DICE_IS_EXCEPTION(env, CORBA_NO_EXCEPTION)


#endif // CORBA_Environment_typedef

/*
 * The helper function section.
 *
 * Declarations first
 */

#ifndef DICE_INLINE
#ifdef __GNUC__
#define DICE_INLINE static inline
#else
#define DICE_INLINE inline
#endif /* __GNUC__ */
#endif /* !DICE_INLINE */

#ifndef DICE_EXTERN_INLINE
#ifdef __GNUC__
#define DICE_EXTERN_INLINE extern inline
#else
#define DICE_EXTERN_INLINE inline
#endif /* __GNUC__ */
#endif /* !DICE_EXTERN_INLINE */


NAMESPACE_DICE_BEG

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

#if defined(L4API_l4v2) || defined(L4API_l4x0) || defined(L4API_l4x2) || defined(L4API_l4v4)

/**
 * \brief set scheduling parameters in the environment
 * \ingroup dice_types
 *
 * \param env        environment to manipulate
 *
 * On V2/Fiasco the default behaviour is to donate the timeslice. Therefore,
 * setting donation bits actually means to unset the no-donate bit.
 *
 * For X2/Pistachio only the default behaviour (implicit time donation) works.
 */
DICE_INLINE void
dice_l4_sched_donate(CORBA_Environment *env);

/**
 * \brief set scheduling parameters in the environment
 * \ingroup dice_types
 *
 * \param env        environment to manipulate
 *
 * On V2/Fiasco the default behaviour is to donate the timeslice. Therefore,
 * setting deceit bits actually will mean to not donate time.
 *
 * For X2/Pistachio only the default behaviour (implicit time donation) works.
 */
DICE_INLINE void
dice_l4_sched_nodonate(CORBA_Environment *env);

/**
 * \brief set scheduling parameters in the server environment
 * \ingroup dice_types
 *
 * \param env        environment to manipulate
 *
 * On V2/Fiasco the default behaviour is to donate the timeslice. Therefore,
 * setting donation bits actually means to unset the no-donate bit.
 *
 * For X2/Pistachio only the default behaviour (implicit time donation) works.
 */
DICE_INLINE void
dice_l4_sched_donate_srv(CORBA_Server_Environment *env);

/**
 * \brief modify the environment to not donate time-slices
 * \ingroup dice_types
 *
 * \param env        environment to manipulate
 *
 * On V2/Fiasco the default behaviour is to donate the timeslice. Therefore,
 * setting deceit bits actually will mean to not donate time.
 *
 * For X2/Pistachio only the default behaviour (implicit time donation) works.
 */
DICE_INLINE void
dice_l4_sched_nodonate_srv(CORBA_Server_Environment *env);

#endif /* L4API */

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

NAMESPACE_DICE_END /* exclude included files */

#include <l4/sys/ipc.h> /* for L4_IPC_DECEIT_MASK */

NAMESPACE_DICE_BEG

DICE_INLINE void
dice_l4_sched_donate(CORBA_Environment *env)
{
    env->_p.sched_bits &= ~L4_IPC_DECEIT_MASK;
}

DICE_INLINE void
dice_l4_sched_nodonate(CORBA_Environment *env)
{
    env->_p.sched_bits |= L4_IPC_DECEIT_MASK;
}

DICE_INLINE void
dice_l4_sched_donate_srv(CORBA_Server_Environment *env)
{
    env->_p.sched_bits &= ~L4_IPC_DECEIT_MASK;
}

DICE_INLINE void
dice_l4_sched_nodonate_srv(CORBA_Server_Environment *env)
{
    env->_p.sched_bits |= L4_IPC_DECEIT_MASK;
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
dice_l4_sched_donate(CORBA_Environment *env)
{}

DICE_INLINE void
dice_l4_sched_donate_srv(CORBA_Server_Environment *env)
{}

DICE_INLINE void
dice_l4_sched_nodonate(CORBA_Environment *env)
{}

DICE_INLINE void
dice_l4_sched_nodonate_srv(CORBA_Server_Environment *env)
{}

#endif /* L4API_* */

NAMESPACE_DICE_END

#ifdef __cplusplus
using namespace dice;
#endif

#endif // __DICE_CORBA_TYPES_H__

