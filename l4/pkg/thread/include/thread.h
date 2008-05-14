/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/include/l4/thread/thread.h
 * \brief  L4 thread library public API 
 *
 * \date   09/02/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _THREAD_THREAD_H
#define _THREAD_THREAD_H

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/l4int.h>
#include <l4/env/cdefs.h>

/*****************************************************************************
 *** data types                                                                
 *****************************************************************************/

/**
 * Thread id type. 
 * \ingroup api_create
 */
typedef l4_int32_t l4thread_t;

/**
 * Thread priority type.
 * \ingroup api_prio
 *
 * The valid priority range depends on the L4 kernel, currently it is 0-255.
 */
typedef l4_int32_t l4_prio_t;

/**
 * Thread function type.
 * \ingroup api_create
 *
 * \param   data         Argument pointer, a user can specify a data argument 
 *                       on l4thread_create() / l4thread_create_long(), it is 
 *                       passed to the new thread in this data pointer.
 */
typedef L4_CV void (* l4thread_fn_t) (void * data);

/**
 * Exit functions (see l4thread_on_exit()).
 * \ingroup api_exit
 *
 * \param   thread       Thread which exists 
 * \param   data         Data pointer
 */
typedef L4_CV void (* l4thread_exit_fn_t) (l4thread_t thread, void * data);

/**
 * Exit functions descriptor, see #L4THREAD_EXIT_FN
 * \ingroup api_exit
 */
typedef struct l4thread_exit_desc
{
  l4thread_exit_fn_t          fn;    ///< exit function
  void *                      data;  ///< data pointer
  struct l4thread_exit_desc * next;  ///< next exit function in list
} l4thread_exit_desc_t;

/*****************************************************************************
 *** defines                                                                   
 *****************************************************************************/

/* thread id */
#define L4THREAD_INVALID_ID    (-1)           /**< \ingroup api_create
					       **  Invalid thread id
					       **/

/* priorities */
#define L4THREAD_DEFAULT_PRIO  (-1)           /**< \ingroup api_create
					       **  Use default priority
					       **/
/* stack */
#define L4THREAD_INVALID_SP (l4_addr_t)(-1)   /**< \ingroup api_create
					       **  Invalid stack pointer
					       **/
#define L4THREAD_DEFAULT_SIZE (l4_size_t)(-1) /**< \ingroup api_create
					       **  Use default stack size
					       **/

/* flags thread creation */
#define L4THREAD_CREATE_SYNC   0x00000001     /**< \ingroup api_create
					       **  Wait for thread startup
					       **/
#define L4THREAD_CREATE_ASYNC  0x00000002     /**< \ingroup api_create
					       **  Don't wait for thread startup
					       **/

#define L4THREAD_CREATE_PINNED 0x20000000     /**< \ingroup api_create
					       **  Create stack on pinned memory
					       **/
#define L4THREAD_CREATE_MAP    0x40000000     /**< \ingroup api_create
					       **  Premap thread stack 
					       **/
#define L4THREAD_CREATE_SETUP  0x80000000     ///< \ingroup api_create 

#define L4THREAD_NAME_LEN      16	      /**< \ingroup api_create
                                               **  max len of a thread name
					       **  including '\0'
                                               **/

/*****************************************************************************
 *** macros
 *****************************************************************************/

/**
 * Declare exit function, generic version
 * \ingroup api_exit
 * 
 * \param   vis          Scope (global or static)
 * \param   name         Funtion name to declare
 * \param   fn           Exit function
 */
#define L4THREAD_EXIT_FN_(vis, name, fn) \
  vis l4thread_exit_desc_t name = { fn, NULL, NULL}

/**
 * Declare exit function (global scope)
 * \ingroup api_exit
 * 
 * \param   name         Funtion name to declare
 * \param   fn           Exit function
 */
#define L4THREAD_EXIT_FN(name, fn)         L4THREAD_EXIT_FN_(,name,fn)

/**
 * Declare exit function (static scope)
 * \ingroup api_exit
 * 
 * \param   name         Funtion name to declare
 * \param   fn           Exit function
 */
#define L4THREAD_EXIT_FN_STATIC(name, fn)  L4THREAD_EXIT_FN_(static,name,fn)

/*****************************************************************************
 *** global configuration data
 *****************************************************************************/

/**
 * Maximum number of threads
 * \ingroup api_config
 */
extern const int l4thread_max_threads;

/**
 * Default stack size for new threads
 * \ingroup api_config
 */
extern const l4_size_t l4thread_stack_size;

/**
 * Maximum stack size of new threads
 * \ingroup api_config
 */
extern const l4_size_t l4thread_max_stack;

/**
 * Default priority for new threads
 * \ingroup api_config
 */
extern l4_prio_t l4thread_default_prio;

/**
 * Stack map are astart address
 * \ingroup api_config
 */
extern const l4_addr_t l4thread_stack_area_addr;

/**
 * TCB table map address
 * \ingroup api_config
 */
extern const l4_addr_t l4thread_tcb_table_addr;

/**
 * default basename for thread creation
 */
extern const char *l4thread_basename;

extern unsigned l4thread_target_cpu;

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

__BEGIN_DECLS;

/*****************************************************************************
 *** create
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Create new thread (standard short form)
 * \ingroup api_create
 * 
 * \param   func         thread function
 * \param   data         data argument which is passed to the new thread
 * \param   flags        create flags
 *                       - #L4THREAD_CREATE_SYNC wait until the new 
 *                         thread is started, the new thread must confirm its 
 *                         startup by calling the l4thread_started() function.
 *                       - #L4THREAD_CREATE_ASYNC return immediately after 
 *                         creating the thread, the new thread might not be 
 *                         completely initialized when l4thread_create returns
 *                       - #L4THREAD_CREATE_PINNED use pinned (non-paged) memory
 *                         to alloacte stack
 *                       - #L4THREAD_CREATE_MAP immediately map stack memory
 *                       - #L4THREAD_CREATE_SETUP use direct calls to 
 *                         the region mapper.
 *                         Note: This flag is inteded to be used by the 
 *                               startup code of a task. It must not be used 
 *                               by application threads.
 *
 * \return  Thread id on success (> 0), error code otherwise:
 *          - -#L4_ENOTHREAD  no thread available
 * 
 * l4thread_create() creates a new thread using the default values for the
 * stack size an priority of the new thread. The name of the thread will
 * be generated from #l4thread_basename by adding a dot (.) and the 0-padded
 * id of the generated thread in hex notation.
 *
 * When \c func returns, the created thread will exit.
 */
/*****************************************************************************/ 
L4_CV l4thread_t 
l4thread_create(l4thread_fn_t func, void * data, l4_uint32_t flags);

/*****************************************************************************/
/**
 * \brief   Create new thread with name
 * \ingroup api_create
 * 
 * \param   func         thread function
 * \param   name         thread name
 * \param   data         data argument which is passed to the new thread
 * \param   flags        create flags
 *                       - #L4THREAD_CREATE_SYNC wait until the new 
 *                         thread is started, the new thread must confirm its 
 *                         startup by calling the l4thread_started() function.
 *                       - #L4THREAD_CREATE_ASYNC return immediately after 
 *                         creating the thread, the new thread might not be 
 *                         completely initialized when l4thread_create returns
 *                       - #L4THREAD_CREATE_PINNED use pinned (non-paged) memory
 *                         to alloacte stack
 *                       - #L4THREAD_CREATE_MAP immediately map stack memory
 *                       - #L4THREAD_CREATE_SETUP use direct calls to 
 *                         the region mapper.
 *                         Note: This flag is inteded to be used by the 
 *                               startup code of a task. It must not be used 
 *                               by application threads.
 *
 * \return  Thread id on success (> 0), error code otherwise:
 *          - -#L4_ENOTHREAD  no thread available
 * 
 * l4thread_create() creates a new thread using the default values for the
 * stack size and priority of the new thread.
 *
 * When \c func returns, the created thread will exit.
 */
/*****************************************************************************/ 
L4_CV l4thread_t 
l4thread_create_named(l4thread_fn_t func, const char*name,
		      void * data, l4_uint32_t flags);

/*****************************************************************************/
/**
 * \brief   Create new thread (long form). 
 * \ingroup api_create
 * 
 * \param   thread       thread id of the new thread, if set to 
 *                       #L4THREAD_INVALID_ID, l4thread_create_long will 
 *                       choose an unused thread.
 * \param   name         name of the thread. If 0, a name will be generated.
 *			 If starting with ".", will be padded to basename.
 *                       Otherwise contents will be copied.
 * \param   func         thread function
 * \param   name         name of the thread. If 0, a name will be generated.
 *			 If starting with ".", will be padded to basename.
 *                       Otherwise contents will be copied. Additionally,
 *			 one "%d" or "%x" in the string is substituted by
 *                       sprintf, withh the thread-id as one argument.
 * \param   stack_pointer stack pointer, if set to #L4THREAD_INVALID_SP, 
 *                       a stack will be allocated
 * \param   stack_size   size of the stack (bytes), if set to 
 *                       #L4THREAD_DEFAULT_SIZE the default size will be 
 *                       used 
 *                       Note: if a stack pointer is specified, a valid 
 *                             stack size must be given.
 * \param   prio         L4 priority of the thread, if set to 
 *                       #L4THREAD_DEFAULT_PRIO, the default priority 
 *                       will be used.
 * \param   data         data argument which is passed to the new thread
 * \param   flags        create flags
 *                       - #L4THREAD_CREATE_SYNC wait until the new thread
 *                         is started, the new thread must confirm its startup
 *                         by calling the l4thread_started() function.
 *                       - #L4THREAD_CREATE_ASYNC return immediately after 
 *                         creating the thread, the new thread might not be 
 *                         completely initialized when l4thread_create_long 
 *                         returns
 *                       - #L4THREAD_CREATE_PINNED use pinned (non-paged) memory
 *                         to alloacte stack
 *                       - #L4THREAD_CREATE_MAP immediately map stack memory
 *                       - #L4THREAD_CREATE_SETUP use direct calls to 
 *                         the region mapper.
 *                         Note: This flag is inteded to be used by the 
 *                               startup code of a task. It must not be used 
 *                               by application threads.
 *
 * \return  Thread id on success (> 0), error code otherwise:
 *          - -#L4_EINVAL     invalid argument
 *          - -#L4_EUSED      thread already used
 *          - -#L4_ENOTHREAD  no thread available
 *          - -#L4_ENOMEM     out of memory allocating stack
 *          - -#L4_ENOMAP     no area found to map stack
 *
 * When \c func returns, the created thread will exit.
 */
/*****************************************************************************/ 
L4_CV l4thread_t 
l4thread_create_long(l4thread_t thread, l4thread_fn_t func,
		     const char * name,
		     l4_addr_t stack_pointer, l4_size_t stack_size,
		     l4_prio_t prio, void * data, l4_uint32_t flags);

/*****************************************************************************/
/**
 * \brief   Send startup notification.
 * \ingroup api_create
 *
 * \param   data         Startup return data, it is stored internally and can 
 *                       be read at any time using l4thread_startup_return().
 *
 * \return  0 on success, error code otherwise:
 *          - -#L4_EIPC IPC error sending startup notification
 *
 * Send startup notification to parent thread. This function must be used to
 * notify the parent thread about the startup of a child thread if the 
 * #L4THREAD_CREATE_SYNC flag was set in l4thread_create() / 
 * l4thread_create_long().
 */
/*****************************************************************************/ 
L4_CV int
l4thread_started(void * data);

/*****************************************************************************/
/**
 * \brief   Get startup return data
 * \ingroup api_create
 *
 * \param   thread       Thread id
 *	
 * \return  Startup data (set with l4thread_started()), 
 *          #NULL if invalid thread id
 */
/*****************************************************************************/ 
L4_CV void *
l4thread_startup_return(l4thread_t thread);

/*****************************************************************************/
/**
 * \brief   Setup thread.
 * \ingroup api_create
 * 
 * \param   l4_id        L4 thread id of the thread
 * \param   name         name of the thread. If 0, a name will be generated.
 *			 If starting with ".", will be padded to basename.
 *                       Otherwise contents will be copied. Additionally,
 *			 one "%d" or "%x" in the string is substituted by
 * \param   stack_low    stack start address
 * \param   stack_high   stack end address
 * 
 * \return  Thread id on success (> 0), error code otherwise:
 *          - -#L4_EINVAL  invalid L4 thread id
 *          - -#L4_EUSED   thread specified by \a l4_id already used by the
 *                         thread lib, or error registering the new name
 *
 * Setup thread descriptor for thread \a l4_id. It must be used to register 
 * threads which are not created by the thread library.
 */
/*****************************************************************************/ 
L4_CV l4thread_t
l4thread_setup(l4_threadid_t l4_id,  const char * name, l4_addr_t stack_low,
	       l4_addr_t stack_high);

/*****************************************************************************
 *** shutdown / exit
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Shutdown thread.
 * \ingroup api_exit
 * 
 * \param   thread       Thread id of thread to shutdown
 * 
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL  invalid thread
 * 
 * Shutdown thread \a thread. All allocated resources are released (stack, 
 * thread control block) and the L4 thread is blocked.
 */
/*****************************************************************************/ 
int
l4thread_shutdown(l4thread_t thread);

/*****************************************************************************/
/**
 * \brief   Exit current thread.
 * \ingroup api_exit
 * 
 * Exit current thread, it is equivalent to
 * l4thread_shutdown(l4thread_myself()), but indicates to the compiler
 * that it does not return.
 */
/*****************************************************************************/ 
L4_CV void
l4thread_exit(void) __attribute__((noreturn));

/*****************************************************************************/
/**
 * \brief   Register exit function for current thread
 * \ingroup api_exit
 *
 * \param   name         Exit function descriptor, it must be declared with the 
 *                       #L4THREAD_EXIT_FN macros
 * \param   data         Data pointer which will be passed to the exit 
 *                       function
 *	
 * \return  0 on success, error code otherwise
 *          - -#L4_EINVAL  invalid error function / thread
 *
 * Register exit function. Threads can have more than one exit function, 
 * they will be called in the reverse order of their registration.
 * The same exit function can be used several times, but for each use a 
 * separate exit function descriptor mus be defined with the 
 * #L4THREAD_EXIT_FN macros.
 */
/*****************************************************************************/ 
L4_CV int
l4thread_on_exit(l4thread_exit_desc_t * name, void * data);

/*****************************************************************************
 *** suspend / sleep
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Sleep.
 * \ingroup api_sleep
 * 
 * \param   t            time (milliseconds), if set to -1 sleep forever
 *
 * Sleep for \a t milliseconds.
 */
/*****************************************************************************/ 
L4_CV void
l4thread_sleep(l4_uint32_t t);

/*****************************************************************************/
/**
 * \brief   Sleep (microseconds)
 * \ingroup api_sleep
 * 
 * \param   t            time (microseconds)
 *
 * Sleep for \a t microseconds, the actual timer resolution depends on the 
 * L4 kernel, common values are 1 or 2 milliseconds.
 */
/*****************************************************************************/ 
L4_CV void 
l4thread_usleep(l4_uint32_t t);

/*****************************************************************************/
/**
 * \brief   Sleep forever.
 * \ingroup api_sleep
 *
 * Sleep forever.
 */
/*****************************************************************************/ 
L4_CV void
l4thread_sleep_forever(void);

/*****************************************************************************
 *** priorities 
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Get priority.
 * \ingroup api_prio
 * 
 * \param   thread       Thread id.
 * 
 * \return  Priority of thread (>= 0), error code otherwise (< 0):
 *          - -#L4_EINVAL invalid thread id
 *
 * Return the L4 priority of thread \a thread.
 */
/*****************************************************************************/ 
L4_CV l4_prio_t 
l4thread_get_prio(l4thread_t thread);

/*****************************************************************************/
/**
 * \brief   Set priority.
 * \ingroup api_prio
 * 
 * \param   thread       Thread id.
 * \param   prio         New priority.
 * 
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL Invalid thread id or priority.
 *
 * Set the L4 priority of \a thread to \a prio.
 */
/*****************************************************************************/ 
L4_CV int 
l4thread_set_prio(l4thread_t thread, l4_prio_t prio);

/*****************************************************************************
 *** thread data
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Allocate new data key.
 * \ingroup api_data
 * 
 * \return  New data key, -#L4_ENOKEY if no key available.
 */
/*****************************************************************************/ 
L4_CV int 
l4thread_data_allocate_key(void);

/*****************************************************************************/
/**
 * \brief   Release data key.
 * \ingroup api_data
 * 
 * \param   key          data key 
 */
/*****************************************************************************/ 
L4_CV void 
l4thread_data_release_key(int key);

/*****************************************************************************/
/**
 * \brief   Set data pointer for current thread
 * \ingroup api_data
 *
 * \param   key          data key
 * \param   data         data pointer
 *	
 * \return  0 on success, -#L4_EINVAL if invalid or unused data key.
 */
/*****************************************************************************/ 
L4_CV int 
l4thread_data_set_current(int key, void * data);

/*****************************************************************************/
/**
 * \brief   Get data pointer for current thread
 * \ingroup api_data
 *
 * \param   key          data key
 *	
 * \return  Data pointer, NULL if invalid or unused data key.
 */
/*****************************************************************************/ 
L4_CV void *
l4thread_data_get_current(int key);

/*****************************************************************************/
/**
 * \brief   Set data pointer
 * \ingroup api_data 
 *
 * \param   thread       thread id
 * \param   key          data key
 * \param   data         data pointer
 *	
 * \return  0 on success, -#L4_EINVAL if invalid or unused data key / thread.
 */
/*****************************************************************************/ 
L4_CV int
l4thread_data_set(l4thread_t thread, int key, void * data);

/*****************************************************************************/
/**
 * \brief   Get data pointer
 * \ingroup api_data
 * 
 * \param   thread       thread id
 * \param   key          data key
 *	
 * \return  Data pointer, NULL if invalid or unused data key / thread.
 */
/*****************************************************************************/ 
L4_CV void *
l4thread_data_get(l4thread_t thread, int key);

/*****************************************************************************
 *** miscellaneous
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Check if two thread ids are equal.
 * \ingroup api_misc
 * 
 * \param   t1           Thread id 1
 * \param   t2           Thread id 2
 * 
 * \return  1 if threads are equal, 0 otherwise
 *
 * Check if the threads \a t1 and \a t2 are equal.
 */
/*****************************************************************************/ 
L4_CV L4_INLINE int
l4thread_equal(l4thread_t t1, l4thread_t t2);

/*****************************************************************************/
/**
 * \brief   Return thread id of current thread.
 * \ingroup api_misc
 * 
 * \return  thread id of the current thread.
 *
 * Return id of the thread calling the function.
 */
/*****************************************************************************/ 
L4_CV l4thread_t 
l4thread_myself(void);

/*****************************************************************************/
/**
 * \brief   Return L4 thread id. 
 * \ingroup api_misc
 * 
 * \param   thread       Thread id
 *
 * \return  L4 thread id, #L4_INVALID_ID if \a thread is an unused or invalid
 *          thread.
 *
 * Return the L4 thread id of the thread \a thread.
 */
/*****************************************************************************/ 
L4_CV l4_threadid_t 
l4thread_l4_id(l4thread_t thread);

/*****************************************************************************/
/**
 * \brief   Return thread id for L4 thread
 * \ingroup api_misc
 * 
 * \param   id           L4 thread id
 *	
 * \return Thread id.
 */
/*****************************************************************************/ 
L4_INLINE l4thread_t
l4thread_id(l4_threadid_t id);

/*****************************************************************************/
/**
 * \brief   Get thread id of parent thread.
 * \ingroup api_misc
 * 
 * \return  Thread id of parent thread, #L4THREAD_INVALID_ID if parent not 
 *          exists.
 */
/*****************************************************************************/ 
L4_CV l4thread_t 
l4thread_get_parent(void);

/*****************************************************************************
 *** lock threads
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Lock thread, this avoids manipulations by other threads, 
 *          especially that the current thread gets killed by someone else
 * \ingroup api_misc
 * 
 * \param   thread       Thread id
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL invalid thread id
 */
/*****************************************************************************/ 
L4_CV int
l4thread_lock(l4thread_t thread);

/*****************************************************************************/
/**
 * \brief   Unlock thread
 * \ingroup api_misc
 * 
 * \param   thread       Thread id
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL invalid thread id
 */
/*****************************************************************************/ 
L4_CV int
l4thread_unlock(l4thread_t thread);

/*****************************************************************************/
/**
 * \brief   Lock current thread, this avoids manipulations by other threads, 
 *          especially that the current thread gets killed by someone else
 * \ingroup api_misc
 *
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL  current thread not found in thread table
 */
/*****************************************************************************/ 
L4_CV int
l4thread_lock_myself(void);

/*****************************************************************************/
/**
 * \brief   Unlock current thread
 * \ingroup api_misc
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL  current thread not found in thread table
 */
/*****************************************************************************/ 
L4_CV int
l4thread_unlock_myself(void);

/*****************************************************************************
 *** thread stacks
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Return stack address
 * \ingroup api_misc 
 * 
 * \param   thread       Thread id
 * \retval  stack_low    Stack address low 
 * \retval  stack_high   Stack address high
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL  invalid thread id
 */
/*****************************************************************************/ 
L4_CV int
l4thread_get_stack(l4thread_t thread, l4_addr_t * low, l4_addr_t * high);

/*****************************************************************************/
/**
 * \brief   Return stack address of current thread
 * \ingroup api_misc 
 * 
 * \retval  stack_low    Stack address low 
 * \retval  stack_high   Stack address high
 *	
 * \return  0 on success, error code otherwise:
 *          - -#L4_EINVAL  current thread not found in thread table
 */
/*****************************************************************************/ 
L4_CV int
l4thread_get_stack_current(l4_addr_t * low, l4_addr_t * high);

/*****************************************************************************/
/**
 * \brief   Dump threads to stdio
 * \ingroup api_misc 
 */
/*****************************************************************************/ 
L4_CV void
l4thread_dump_threads(void);

/*****************************************************************************
 *** library setup                                                         
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief   Init L4 thread library.
 * \ingroup api_config
 *
 * Initialize thread library. This function is usually call by the setup 
 * routine of a task, application threads must not use it.
 */
/*****************************************************************************/ 
L4_CV int
l4thread_init(void);

__END_DECLS;

/*****************************************************************************
 *** implementation
 *****************************************************************************/

/*****************************************************************************
 *** l4thread_id
 *****************************************************************************/
L4_INLINE l4thread_t
l4thread_id(l4_threadid_t id)
{
  return id.id.lthread;
}

/*****************************************************************************
 *** l4thread_equal
 *****************************************************************************/
L4_CV L4_INLINE int
l4thread_equal(l4thread_t t1, l4thread_t t2)
{
  return (t1 == t2) ? 1 : 0;
}

#endif /* !_THREAD_THREAD_H */
