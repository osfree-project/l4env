/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/src/create.c
 * \brief  Create threads.
 *
 * \date   09/03/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* standard includes */
#include <stdio.h>
#include <string.h>

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/names/libnames.h>

/* private includes */
#include <l4/thread/thread.h>
#include "__tcb.h"
#include "__l4.h"
#include "__thread.h"
#include "__stacks.h"
#include "__prio.h"
#include "__debug.h"

/*****************************************************************************
 *** thread startup
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Start new thread
 *
 * This function cannot be instrumented, as it has no upper stack (caller).
 * Call-graph analysis would raise pagefaults.
 */
/*****************************************************************************/
void l4th_thread_start(void) L4_NOINSTRUMENT;
void
l4th_thread_start(void)
{
  l4th_tcb_t * me;

  /* get current tcb */
  me = l4th_tcb_get_current();
  if (me == NULL)
    {
      /* Ooops, invalid tcb of current thread. This can only happen if the
       * new thread already gets killed (thus the state of the tcb is set to
       * TCB_SHUTDWON), but the killer did not yet set us to the exit handler
       * (see l4thread_shutdown()). Just wait for the shutdown to happen.
       */
      LOG_Error("invalid TCB (already got killed!?). Wait for shutdown...");
      l4thread_sleep_forever();
    }

  LOGdL(DEBUG_CREATE, "I am at %p, "l4util_idfmt" (l4th: %x)",
        me, l4util_idstr(me->l4_id), me->id);

  /* call thread function */
  me->func(me->startup_data);

  /* exit thread */
  l4thread_exit();
}

/*****************************************************************************
 *** startup notification
 *****************************************************************************/

/// startup notification magic key
#define L4THREAD_STARTUP_MAGIC       0x0abacab0

/*****************************************************************************/
/**
 * \brief  Send startup notification
 *
 * \param  tcb           Thread control block of current thread
 *
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL invalid parent entry in tcb
 *         - -#L4_EIPC   IPC error sending startup notification
 *
 * Send startup notification to the parent thread of \a tcb.
 */
/*****************************************************************************/
static inline int
__send_startup_notification(l4th_tcb_t * tcb)
{
  int error;
  l4_msgdope_t result;
  l4_umword_t dw0,dw1;

  if (tcb->parent == NULL)
    {
      LOG_Error("l4thread: failed to get parent thread!");
      return -L4_EINVAL;
    }

  LOGdL(DEBUG_STARTUP,"child:  %2d ("l4util_idfmt")",
        tcb->id, l4util_idstr(tcb->l4_id));
  LOGdL(DEBUG_STARTUP,"parent: %2d ("l4util_idfmt")",
        tcb->parent->id, l4util_idstr(tcb->parent->l4_id));

  /* send IPC to parent */
  error = l4_ipc_call(tcb->parent->l4_id, L4_IPC_SHORT_MSG,
		      L4THREAD_STARTUP_MAGIC, 0 ,
		      L4_IPC_SHORT_MSG, &dw0, &dw1,
		      L4_IPC_NEVER, &result);
  if (error || (dw0 != ~L4THREAD_STARTUP_MAGIC))
    {
      LOG_printf("l4thread: Error sending startup notification (%d -> %d)\n",
             tcb->id, tcb->parent->id);
      LOG_Error("l4thread: IPC error %02x, magic 0x%08x -> 0x%08x",
                error, L4THREAD_STARTUP_MAGIC, ~dw0);
      return -L4_EIPC;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Wait for startup notification from child thread
 *
 * \param  child         TCB of child thread
 *
 * \return 0 on success, error code otherwise:
 *         - -#L4_EIPC IPC error waiting for notification
 */
/*****************************************************************************/
static inline int
__wait_for_startup_notification(l4_threadid_t child)
{
  int error;
  l4_umword_t magic,dummy;
  l4_msgdope_t result;

  /* wait */
  error = l4_ipc_receive(child, L4_IPC_SHORT_MSG, &magic, &dummy,
			 L4_IPC_NEVER, &result);
  if (!error)
    error = l4_ipc_send(child, L4_IPC_SHORT_MSG, ~magic, 0,
			L4_IPC_SEND_TIMEOUT_0, &result);
  if (error)
    {
      LOG_Error("l4thread: Error waiting for startup of thread "l4util_idfmt \
                " (%02x)", l4util_idstr(child), error);
      return -L4_EIPC;
    }

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Setup the name of a thread and register at name server
 *
 * \param  tcb           tcb to set up. ID must be filled in.
 * \param  name          name to use. If 0, the name of the thread will
 *			 be generated from #l4thread_basename by adding
 *			 a dot (.) and the 0-padded id of the generated
 *			 thread in hex notation.
 *			 If starting with ".", name will be padded to basename.
 *                       Otherwise contents will be copied. Additionally,
 *                       one "%d" or "%x" in the string is substituted by
 *                       sprintf, withh the thread-id as one argument.
 *
 * \return 0 on success (registered thread name at nameserver), != 0 if
 *         register failed
 *
 * This function copied the name into the tcb, or generates one if none is
 * provided. Then it tries to register the thread at names.
 */
/*****************************************************************************/
static inline int
__setup_name(l4th_tcb_t * tcb, const char * name)
{
  int ret;

  if(name && *name != '.')
    snprintf(tcb->name, L4THREAD_NAME_LEN, name, tcb->id);

  else
    {
      int l;
      l = strlen(l4thread_basename);

      // copy 4 bytes less than max to leave space for '.XX\0'
      if(l > L4THREAD_NAME_LEN - 4)
        l = L4THREAD_NAME_LEN - 4;
      memcpy(tcb->name, l4thread_basename, l);

      if(name)	// first is a dot
	snprintf(tcb->name + l, L4THREAD_NAME_LEN-l, name, tcb->id);

      else
        sprintf(tcb->name + l, ".%02X", tcb->id);
    }

  LOGdL(DEBUG_THREAD_NAME, "register name \'%s\' for thread "l4util_idfmt,
        tcb->name, l4util_idstr(tcb->l4_id));

  ret = names_register_thread_weak(tcb->name, tcb->l4_id);
  if (!ret)
    LOG_Error("register name \'%s\' for thread "l4util_idfmt" failed",
              tcb->name, l4util_idstr(tcb->l4_id));

  return !ret;
}

/*****************************************************************************
 *** create thread
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Create new thread.
 *
 * \param  thread        thread id (#L4THREAD_INVALID_ID .. find unused)
 * \param  func          thread function
 * \param  name          name of the thread. If 0, a name will be generated.
 *			 If starting with ".", will be padded to basename.
 *                       Otherwise contents will be copied. Additionally,
 *                       one "%d" or "%x" in the string is substituted by
 *                       sprintf, withh the thread-id as one argument.
 * \param  stack_pointer stack pointer
 *                       (#L4THREAD_INVALID_SP .. allocate stack)
 * \param  stack_size    stack size
 *                       (#L4THREAD_DEFAULT_SIZE .. use default stack size)
 * \param  prio          thread priority
 *                       (#L4THREAD_DEFAULT_PRIO .. use default priority)
 * \param  data          thread data
 * \param  flags         create flags (see l4/thread/types.h)
 *
 * \return thread id on success (>= 0), error code otherwise:
 *         - -#L4_EINVAL    invalid argument
 *         - -#L4_ENOTHREAD no thread available
 *         - -#L4_EUSED     thread already used
 *         - more see l4th_alloc_pages()
 *
 * This function actually creates a new thread, l4thread_create() and
 * l4thread_create_long() call this function.
 *
 * \todo handle error waiting for startup notification.
 */
/*****************************************************************************/
static l4thread_t
__create(l4thread_t thread, l4thread_fn_t func, const char * name,
	 l4_addr_t stack_pointer, l4_size_t stack_size,
         l4_prio_t prio, void * data, l4_uint32_t flags)
{
  int ret,i;
  l4th_tcb_t * tcb, * me;
  l4_addr_t sp;

  /* sanity checks */
  if ((func == NULL) || (stack_size == 0))
    return -L4_EINVAL;

  /* lock myself, this avoids that someone else destroys us while we are
   * creating the new thread */
  me = l4th_tcb_get_current_locked();
  if (me == NULL)
    return -L4_EINVAL;

  /* allocate tcb, the state of the tcb will be 'TCB_SETUP' (see
   * l4th_tcb_allocate()), thats why the thread is still 'invisible' for
   * other threads. This avoids the (unlikely) race condition where someone
   * else already wants to modify the new thread, modifications are only
   * done if the state is TCB_ACTIVE (see l4th_tcb_get*()).
   */
  if (thread == L4THREAD_INVALID_ID)
    ret = l4th_tcb_allocate(&tcb);
  else
    ret = l4th_tcb_allocate_id(thread,&tcb);
  if (ret < 0)
    {
      l4th_tcb_unlock(me);
      return ret;
    }

  /* lock tcb of new thread */
  l4th_tcb_lock(tcb);

  /* setup tcb */
  tcb->parent = me;
  tcb->prio = prio;
  tcb->l4_id = l4th_l4_to_l4id(me->l4_id, tcb->id);
  if(__setup_name(tcb, name))
    {
      l4th_tcb_unlock(tcb);
      l4th_tcb_free(tcb);
      l4th_tcb_unlock(me);
      return ret;
    }

  LOGdL(DEBUG_CREATE,
	"l4id of newly created thread %d = "l4util_idfmt \
        " (I am %x at %p - "l4util_idfmt")", tcb->id,
        l4util_idstr(tcb->l4_id), me->id, me, l4util_idstr(me->l4_id));

  tcb->flags = 0;
  tcb->exit_fns = NULL;
  for (i = 0; i < L4THREAD_MAX_DATA_KEYS; i++)
    tcb->data[i] = NULL;

  /* allocate stack */
  if (stack_pointer == L4THREAD_INVALID_SP)
    {
      /* allocate stack */
      if (stack_size == L4THREAD_DEFAULT_SIZE)
	ret = l4th_stack_allocate(tcb->id, l4thread_stack_size, flags,
				  tcb->l4_id, &tcb->stack);
      else
	ret = l4th_stack_allocate(tcb->id, stack_size, flags, tcb->l4_id,
				  &tcb->stack);
      if (ret < 0)
	{
	  /* stack allocation failed */
	  LOG_Error("l4thread: stack allocation failed: %s (%d)!",
                    l4env_errstr(ret), ret);

          l4th_tcb_unlock(tcb);
	  l4th_tcb_free(tcb);
	  l4th_tcb_unlock(me);
	  return ret;
	}

      /* prefill the stack for sanity reasons */
#ifdef DEBUG_SANITY
      if (tcb->id >= 3)
        memset((void *)tcb->stack.map_addr, 0x99, tcb->stack.size);
#endif

      tcb->flags |= TCB_ALLOCATED_STACK;
    }
  else
    {
      /* user allocated stack */
      tcb->stack.size = stack_size;
      tcb->stack.map_addr = stack_pointer - stack_size;
    }

  LOGdl(DEBUG_STACK_INIT, "Setting up stack at %p+%d...",
       (void*)tcb->stack.map_addr, tcb->stack.size);

  /* setup stack */
  sp = l4th_thread_setup_stack(tcb->stack.map_addr, tcb->stack.size,
			       tcb->l4_id, flags);

  /* Finished setting up the tcb of the new thread, set state of new thread
   * to active. Setting the state to active makes the thread visible to other
   * threads, but because we still hold the tcb lock of the new thread it
   * cannot yet be modified by other threads until we started the thread and
   * set its priority */
  tcb->state = TCB_ACTIVE;

  /* start thread */
  LOGdL(DEBUG_CREATE,"creating thread %d ("l4util_idfmt")\n" \
        " fn at 0x%08x, stack high at 0x%08x, data 0x%08x",
        tcb->id, l4util_idstr(tcb->l4_id), (unsigned)func, sp, (unsigned)data);

  tcb->func = func;
  tcb->startup_data = data;
  tcb->return_data = NULL;
  l4th_l4_create_thread(tcb->l4_id, (l4_addr_t)l4th_thread_entry, sp,
			l4th_thread_get_pager());

  /* set priority of new thread */
  if (prio == L4THREAD_DEFAULT_PRIO)
    ret = l4th_set_prio(tcb, l4thread_default_prio);
  else
    ret = l4th_set_prio(tcb, prio);
  if (ret < 0)
    /* ignore error, but print error message */
    LOG_Error("l4thread: set priority for new thread failed: %s (%d)!",
              l4env_errstr(ret), ret);

  /* done with creating thread, unlock tcb of the new thread */
  l4th_tcb_unlock(tcb);

  if ((flags & L4THREAD_CREATE_SYNC) && !(flags & L4THREAD_CREATE_SETUP))
    {
      /* sync create, wait for startup notification */
      ret = __wait_for_startup_notification(tcb->l4_id);
      if (ret < 0)
	/* error waiting for startup notification */
	LOG_Error("l4thread: IPC error waiting for startup notification");
    }

  /* done */
  l4th_tcb_unlock(me);

  return tcb->id;
}

/*****************************************************************************
 *** l4thread user API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Create new thread (standard short form)
 *
 * \param  func          Thread function
 * \param  data          Data argument which is passed to the new thread
 * \param  flags         Flags:
 *                       - #L4THREAD_CREATE_SYNC  wait until the new thread
 *                         is started, the new thread must confirm its
 *                         startup by calling the l4thread_started()
 *                         function.
 *                       - #L4THREAD_CREATE_ASYNC return immediately after
 *                         creating the thread
 *                       - #L4THREAD_CREATE_SETUP  Task setup, use direct
 *                         attach call to attach stack dataspaces
 *                         Note: This flag is inteded to be used by the
 *                               startup code of a task. It must not be
 *                               used by application threads.
 *
 * \return Thread id on success (> 0), error code otherwise (see __create)
 *
 * l4_create_thread creates a new thread using the default values for the
 * stack size an priority of the new thread. The name of the thread will
 * be generated from #l4thread_basename by adding a dot (.) and the 0-padded
 * id of the generated thread in hex notation.
 *
 * When \c func returns, the created thread will exit.
 */
/*****************************************************************************/
l4thread_t
l4thread_create(l4thread_fn_t func, void * data, l4_uint32_t flags)
{
  /* create thread */
  return __create(L4THREAD_INVALID_ID, func, 0, L4THREAD_INVALID_SP,
		  L4THREAD_DEFAULT_SIZE, L4THREAD_DEFAULT_PRIO, data, flags);
}

/*****************************************************************************/
/**
 * \brief  Create new thread with name
 * \see l4thread_create
 */
/*****************************************************************************/
l4thread_t
l4thread_create_named(l4thread_fn_t func, const char*name,
		      void * data, l4_uint32_t flags)
{
  /* create thread */
  return __create(L4THREAD_INVALID_ID, func, name, L4THREAD_INVALID_SP,
		  L4THREAD_DEFAULT_SIZE, L4THREAD_DEFAULT_PRIO, data, flags);
}

/*****************************************************************************/
/**
 * \brief  Create new thread (long form).
 *
 * \param  thread        Thread id of the new thread, if set to
 *                       #L4THREAD_INVALID_ID, l4thread_create_long will
 *                       choose an unused thread.
 * \param  func          Thread function
 * \param  name          name of the thread. If 0, a name will be generated.
 *			 If starting with ".", will be padded to basename.
 *                       Otherwise contents will be copied.
 * \param  stack_pointer Start address of the stack, if set to
 *                       #L4THREAD_INVALID_SP, the stack will be allocated
 * \param  stack_size    Size of the stack, if set to #L4THREAD_DEFAULT_SIZE
 *                       the default size will be used (Note: if a stack
 *                       pointer is specified, a valid stack size must be
 *                       given).
 * \param  prio          L4 priority of the thread, if set to
 *                       #L4THREAD_DEFAULT_PRIO, the default priority will
 *                       be used.
 * \param  data          Data argument which is passed to the new thread
 * \param  flags         Flags:
 *                       - #L4THREAD_CREATE_SYNC  wait until the new thread
 *                         is started, the new thread must confirm its
 *                         startup by calling the l4thread_started()
 *                         function.
 *                       - #L4THREAD_CREATE_ASYNC return immediately after
 *                         creating the thread
 *                       - #L4THREAD_CREATE_SETUP  Task setup, use direct
 *                         attach call to attach stack dataspaces
 *                         Note: This flag is inteded to be used by the
 *                               startup code of a task. It must not be
 *                               used by application threads.
 *
 * \return Thread id on success (> 0), error code otherwise (see __create)
 *
 * When \c func returns, the created thread will exit.
 */
/*****************************************************************************/
l4thread_t
l4thread_create_long(l4thread_t thread, l4thread_fn_t func,
		     const char *name, l4_addr_t stack_pointer,
                     l4_size_t stack_size, l4_prio_t prio,
                     void * data, l4_uint32_t flags)
{
  /* create thread */
  return __create(thread, func, name, stack_pointer, stack_size,
                  prio, data, flags);
}

/*****************************************************************************/
/**
 * \brief  Send startup notification.
 *
 * \param  data          Startup return data, it is returned to the parent
 *                       thread in \a ret_data (see l4thread_create()).
 *
 * \return 0 on success, error code otherwise:
 *         - -#L4_EIPC IPC error sending startup notification
 *
 * Send startup notification to parent thread.
 */
/*****************************************************************************/
int
l4thread_started(void * data)
{
  l4th_tcb_t * tcb;
  int ret;

  /* get tcb */
  tcb = l4th_tcb_get_current_locked();
  if ((tcb == NULL) || (tcb->state != TCB_ACTIVE))
    {
      LOG_Error("l4thread: failed to get current tcb!");
      return -L4_EINVAL;
    }

  /* set startup return data in tcb */
  tcb->return_data = data;

  /* send notification */
  ret = __send_startup_notification(tcb);

  /* done */
  l4th_tcb_unlock(tcb);
  return ret;
}

/*****************************************************************************/
/**
 * \brief  Get startup return data
 *
 * \param  thread        Thread id
 *
 * \return Startup data, #NULL if invalid thread id
 */
/*****************************************************************************/
void *
l4thread_startup_return(l4thread_t thread)
{
  l4th_tcb_t * tcb;

  /* get TCB */
  tcb = l4th_tcb_get(thread);
  if (tcb == NULL)
    return NULL;
  else
    return tcb->return_data;
}

/*****************************************************************************/
/**
 * \brief  Setup thread.
 *
 * \param  l4_id         L4 thread id of the thread
 * \param  stack_low     Stack start address
 * \param  stack_high    Stack end address
 *
 * \return Thread id on success (> 0), error code otherwise:
 *         - -#L4_EINVAL  invalid L4 thread id
 *         - -#L4_EUSED   thread specified by \a l4_id already used by the
 *                        thread lib, or error registering the new name
 *
 * Setup thread descriptor for thread \a l4_id. It must be used to register
 * threads which are not created by the thread library.
 */
/*****************************************************************************/
l4thread_t
l4thread_setup(l4_threadid_t l4_id, const char * name,
	       l4_addr_t stack_low, l4_addr_t stack_high)
{
  l4thread_t id;
  l4th_tcb_t * tcb;
  int ret;

  LOGdL(DEBUG_CREATE, "thread "l4util_idfmt"\n  stack 0x%08x-0x%08x\n",
        l4util_idstr(l4_id), stack_low, stack_high);

  /* setup tcb */
  id = l4th_l4_from_l4id(l4_id);
  ret = l4th_tcb_allocate_id(id, &tcb);
  if ((ret < 0) && (ret != -L4_ENOTAVAIL))
    /* tcb allocation failed, thread already used?
     * -L4_ENOTAVAIL is a special case, it means that the allocation of
     * the thread id for the TCB failed, this can happen if we use an external
     * thread id management like in the thread_linux_kernel emulation
     * (see thread_linux_kernel/lib/src/tcb_linux_kernel.c).
     * In this case we still setup the TCB so the thread can use the
     * threadlib */
    return ret;

  tcb->flags = 0;
  tcb->l4_id = l4_id;
  tcb->parent = NULL;
  tcb->stack.map_addr = stack_low;
  tcb->stack.size = stack_high - stack_low;
  tcb->startup_data = NULL;
  tcb->return_data = NULL;
  tcb->exit_fns = NULL;

  if(__setup_name(tcb, name))
    {
      l4th_tcb_free(tcb);
      return -L4_EUSED;
    }

  /* get priority */
  tcb->prio = L4THREAD_DEFAULT_PRIO;
  l4th_get_prio(tcb);

  /* make thread visible */
  tcb->state = TCB_ACTIVE;

  /* done */
  return id;
}
