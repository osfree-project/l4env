/*****************************************************************************/
/**
 * \file    l4sys/include/ARCH-amd64/L4API-l4v2/syscalls.h
 * \brief   L4 System calls (except for IPC)
 * \ingroup api_calls
 */
/*****************************************************************************/
#ifndef __L4_SYSCALLS_H__
#define __L4_SYSCALLS_H__

#include <l4/sys/types.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/utcb.h>

/*****************************************************************************
 *** Syscall arguments
 *****************************************************************************/

/* l4_fpage_unmap */
#define L4_FP_REMAP_PAGE        0x00    /**< Set page to read only
                                         **  \ingroup api_calls_other
                                         **  \hideinitializer
                                         **/
#define L4_FP_FLUSH_PAGE        0x02    /**< Flush page completely
                                         **  \ingroup api_calls_other
                                         **  \hideinitializer
                                         **/

#define L4_FP_OTHER_SPACES      0x00    /**< Flush page in all other
                                         **  address spaces
                                         **  \ingroup api_calls_other
                                         **  \hideinitializer
                                         **/
#define L4_FP_ALL_SPACES        0x80000000U
                                        /**< Flush page in own address
                                         **  space too
                                         **  \ingroup api_calls_other
                                         **  \hideinitializer
                                         **/

/* l4_nchief return values */
#define L4_NC_SAME_CLAN         0x00    /**< Destination resides within the
                                         **  same clan
                                         **  \ingroup api_calls_other
                                         **  \hideinitializer
                                         **/
#define L4_NC_INNER_CLAN        0x0C    /**< Destination is in an inner clan
                                         **  \ingroup api_calls_other
                                         **  \hideinitializer
                                         **/
#define L4_NC_OUTER_CLAN        0x04    /**< Destination is outside the
                                         **  invoker's clan
                                         **  \ingroup api_calls_other
                                         **  \hideinitializer
                                         **/

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

/**
 * Unmap flexpage
 * \ingroup api_calls_other
 *
 * \param   fpage        Flexpage descriptor
 * \param   map_mask     Unmap mask (unmap type and unmap location can be
 *                       combined)
 *                       - Unmap type:
 *                         - #L4_FP_REMAP_PAGE remap all pages read-only.
 *                           Pages already mapped read-only are not affected.
 *                         - #L4_FP_FLUSH_PAGE completely unmap pages
 *                       - Unmap location:
 *                         - #L4_FP_OTHER_SPACES unmap pages in all address
 *                           spaces into which pages have been mapped directly
 *                           or indirectly. The pages in the own address space
 *                           remain mapped.
 *                         - #L4_FP_ALL_SPACES additionally, also unmap the
 *                           pages in the own address space.
 *                       - bits 8:19 may contain a task ID for a specific
 *                         unmap in a task, has no effect if
 *                         L4_FP_ALL_SPACES set
 *
 * The l4_fpage_unmap() system call unmaps the specified flex page
 * in all address spaces into which the invoker mapped it directly or
 * indirectly.
 */
L4_INLINE void
l4_fpage_unmap(l4_fpage_t fpage,
	       l4_umword_t map_mask);

/**
 * Get the id of the current thread
 * \ingroup api_calls_other
 *
 * \return  Thread id of the calling thread.
 */
L4_INLINE l4_threadid_t
l4_myself(void);

/**
 * Get the id of the current thread, non-profile version
 * \ingroup api_calls_other
 *
 * \return  Thread id of the calling thread.
 */
L4_INLINE l4_threadid_t
l4_myself_noprof(void);

/**
 * Get the thread id of the nearest partner of the invoker
 * \ingroup api_calls_other
 *
 * \param   destination  Destination thread id
 * \retval  next_chief   Thread id of the nearest partner, depending on
 *                       return value.
 *
 * \return  Destination:
 *          - #L4_NC_SAME_CLAN The destination resides in the same clan,
 *            its id is returned.
 *          - #L4_NC_INNER_CLAN The destination resides in an inner clan,
 *            whose chief is in the same clan as the caller. The call returns
 *            the id of this chief.
 *          - #L4_NC_OUTER_CLAN The destination is outside of the invoker's
 *            clan. It's own chief's id is returned.
 *
 * The l4_nchief() system call delivers the id of the nearest partner
 * which would be engaged when sending a message to the specified destination.
 */
L4_INLINE int
l4_nchief(l4_threadid_t destination,
	  l4_threadid_t *next_chief);

/**
 * Read and write register values of a thread, create a thread
 * \ingroup api_calls_other
 *
 * \param   destination  Destination thread id
 * \param   eip          The new instruction pointer of the thread. It must
 *                       point into the user-accessible part of the address
 *                       space. The existing instruction pointer is not
 *                       modified if \c ~0UL is given.
 * \param   esp          The new stack pointer for the thread. It must point
 *                       into the user-accessible part of the address space.
 *                       The existing stack pointer is not modified if
 *                       \c ~0UL is given.
 * \param   preempter    Defines the internal preempter used by the thread.
 *                       The current preempter id is not modified if
 *                       #L4_INVALID_ID is given.
 * \param   pager        Defines the pager used by the thread. the current
 *                       pager id is not modified if #L4_INVALID_ID is given.
 * \retval  preempter    Id of the old preempter of the destination thread.
 * \retval  pager        Id of the old pager of the destination thread.
 * \retval  old_eflags   Old flags of the destination thread.
 * \retval  old_eip      Old instruction pointer of the destination thread.
 * \retval  old_esp      Old stack pointer of the destination thread.
 *
 * The l4_thread_ex_regs() system call reads and writes user-level
 * register values of a thread in the current task. Ongoing kernel
 * activities are not effected. An IPC operation is canceled or aborted,
 * however. Setting stack and instruction pointer to different valid
 * values results in the creation of a new thread.
 */
L4_INLINE void
l4_thread_ex_regs(l4_threadid_t destination,
		  l4_umword_t eip,
		  l4_umword_t esp,
		  l4_threadid_t *preempter,
		  l4_threadid_t *pager,
		  l4_umword_t *old_eflags,
		  l4_umword_t *old_eip,
		  l4_umword_t *old_esp);

/**
 * Version of l4_thread_ex_regs() that supports additional flags.
 * \ingroup api_calls_other
 *
 * This version of l4_thread_ex_regs() supports additional flags that
 * will not interrupt ongoing IPC (L4_THREAD_EX_REGS_NO_CANCEL),
 * i.e. the effect of l4_thread_ex_regs() will be deferred until the IPC has
 * finished. Note, that if the corresponding IPC is an exception reply, the
 * reply overwrites the values set by l4_thread_ex_regs().
 * The second flag sets the alien bit (L4_THREAD_EX_REGS_ALIEN), i.e. the
 * thread will trigger exceptions whenever it executes a system call.
 */
L4_INLINE void
l4_thread_ex_regs_flags(l4_threadid_t destination,
                        l4_umword_t eip,
                        l4_umword_t esp,
                        l4_threadid_t *preempter,
                        l4_threadid_t *pager,
                        l4_umword_t *old_eflags,
                        l4_umword_t *old_eip,
                        l4_umword_t *old_esp,
		        unsigned long flags);

/**
 * Version of l4_thread_ex_regs() working over task boundaries
 * \ingroup api_calls_other
 *
 * \param destination   See \ref l4_thread_ex_regs, additionally the task id in
 *                      the destination is also considered.
 * \param eip           See \ref l4_thread_ex_regs
 * \param esp           See \ref l4_thread_ex_regs
 * \param preempter     See \ref l4_thread_ex_regs
 * \param pager         See \ref l4_thread_ex_regs
 * \param cap_handler   The new capability handler of the thread. The
 *                      capability handler is not modified if
 *                      #L4_INVALID_ID is given.
 * \param flags         Flags, see \ref l4_thread_ex_regs_flags_options
 * \param utcb          UTCB pointer of the caller.
 *
 * \retval preempter    See \ref l4_thread_ex_regs
 * \retval pager        See \ref l4_thread_ex_regs
 * \retval cap_handler  The old ID of the capability handler of the thread.
 * \retval old_eflags   See \ref l4_thread_ex_regs
 * \retval old_eip      See \ref l4_thread_ex_regs
 * \retval old_esp      See \ref l4_thread_ex_regs
 *
 * \see l4_thread_ex_regs
 */
L4_INLINE void
l4_inter_task_ex_regs(l4_threadid_t destination,
                      l4_umword_t eip,
                      l4_umword_t esp,
                      l4_threadid_t *preempter,
                      l4_threadid_t *pager,
                      l4_threadid_t *cap_handler,
                      l4_umword_t *old_eflags,
                      l4_umword_t *old_eip,
                      l4_umword_t *old_esp,
                      unsigned long flags,
                      l4_utcb_t *utcb);

/**
 * Special version of l4_thread_ex_regs to get the pager of a thread
 * with the callers address space.
 */
L4_INLINE l4_threadid_t
l4_thread_ex_regs_pager(l4_threadid_t destination);

/**
 * Release the processor non-preemptively
 * \ingroup api_calls_other
 *
 * \param   destination  The id of the destination thread the processor
 *                       should switch to:
 *                       - #L4_NIL_ID Processing switches to an undefined
 *                         ready thread which is selected by the scheduler.
 *                       - \c \<valid \c id\> If the destination thread is
 *                         ready, processing switches to this thread. Otherwise
 *                         another ready thread is selected by the scheduler.
 *
 * The l4_thread_switch() system call frees the processor from the
 * invoking thread non-preemptively so that another ready thread can be
 * processed.
 */
L4_INLINE void
l4_thread_switch(l4_threadid_t destination);

/**
 * Release the processor non-preemptively, switch to any thread which is
 * ready to run.
 * \ingroup api_calls_other
 */
L4_INLINE void
l4_yield(void);

/**
 * Define priority, timeslice length and external preempter of other threads.
 * \ingroup api_calls_other
 *
 * \param   dest          The identifier of the destination thread. The
 *                        destination thread must currently exist and run on a
 *                        priority level less than or equal to the current
 *                        thread's mcp (maximum controlled priority).
 * \param   param         Scheduling parameters, see l4_sched_param_t /
 *                        l4_sched_param_struct_t. If #L4_INVALID_SCHED_PARAM,
 *                        is given, the thread's current scheduling parameters
 *                        are not modified.
 * \param   ext_preempter The id of the external preempter for the thread.
 *                        If #L4_INVALID_ID is given, the current external
 *                        preempter of the thread is not changed.
 * \retval  ext_preempter Old external preempter of the destination thread.
 * \retval  partner       Id of a partner of an active user-invoked IPC
 *                        operation. This parameter is only valid, if the
 *                        thread's user state is sending, receiving, pending
 *                        or waiting. #L4_INVALID_ID is delivered if
 *                        there is no specific partner, i.e. if the thread is
 *                        in an open receive state.
 * \retval  old_param     Old scheduling parameter (see l4_sched_param_t /
 *                        l4_sched_param_struct_t). If #L4_INVALID_SCHED_PARAM
 *                        the addressed thread either does not exist or has
 *                        priority which exceeds the current thread's mcp
 *                        (maximum controlled priority).
 *
 * \return  64 Bit value:
 *          - Bits 0-47: CPU time (µs) which has been consumed by the
 *            destination thread
 *          - Bits 48-51: Effective pagefault wakeup of the destination thread
 *            (exponent, encoded like a pagefault timeout).
 *            The value denotes the still remaining timeout interval, valid only
 *            if kernel state is \a pager (see \a old_param).
 *          - Bits 52-55: Current user-level wakeup of the destination thread
 *            (exponent, encoded like a timeout). The value denotes the still
 *            remaining timeout interval, valid only if the user state is
 *            \a waiting or \a pending (see \a old_param).
 *          - Bits 56-63:  Current user-level wakeup of the destination thread
 *            (mantissa).
 *
 * The l4_thread_schedule() system call can be used by schedulers to
 * define priority, timeslice length and external preempter of other
 * threads. Furthermore it delivers thread states.
 *
 * The system call is only effective, if the current priority of the
 * specified destination is less than or equal to the current task's
 * maximum controlled priority (mcp).
 */
L4_INLINE l4_cpu_time_t
l4_thread_schedule(l4_threadid_t dest,
		   l4_sched_param_t param,
		   l4_threadid_t *ext_preempter,
		   l4_threadid_t *partner,
		   l4_sched_param_t *old_param);

/**
 * Create or delete a task
 * \ingroup api_calls_other
 *
 * \param   destination  Task id of an existing task whose chief is the
 *                       current task. If this is not fulfilled, the system
 *                       call has no effect. Simultaneously, a new task with
 *                       the same task number is created. It may be active
 *                       or inactive.
 * \param   mcp_or_new_chief_and_flags
 *                       Depending on the state of the task (active or
 *                       inactive), two variants are possible here:
 *                       - \c \<mcp\> Maximum controlled priority defines
 *                         the highest priority which can be ruled by the new
 *                         task acting as a scheduler. The new task's effective
 *                         mcp is the minimum of the creator's mcp and the
 *                         specified mcp. Use this parameter if the newly
 *                         generated task is an active task.
 *                       - \c \<new_chief\> Specifies the chief of the new
 *                         inactive task. This mechanism permits to transfer
 *                         inactive tasks to other tasks. Transferring an
 *                         inactive task to the specified chief means to
 *                         transfer the related right to create a task. Use
 *                         this parameter if the newly generated task is an
 *                         inactive task.
 *                       .
 *                       Additionally, the following flags may be bitwise
 *                       or'ed with this parameter when starting an active
 *                       task:
 *                       - \c #L4_TASK_NEW_RAISE_EXCEPTION: Raise an
 *                         exception in thread 0 before executing any user
 *                         code. The exception will be sent to the task's
 *                         exception handler.
 *                       - \c #L4_TASK_NEW_ALIEN: Start thread 0 as alien.
 *                         Exceptions are generated for system calls and
 *                         page faults.
 * \param   esp          Initial stack pointer for lthread 0 if the new task is
 *                       created as an active one. Ignore otherwise.
 * \param   eip          Initial instruction pointer for lthread 0 if the new
 *                       task is created as an active one. Ignored otherwise.
 * \param   pager        If #L4_NIL_ID is used, the new task is created as
 *                       inactive. Lthread 0 is not created. Otherwise the new
 *                       task is created as active and the specified pager is
 *                       associated to Lthread 0.
 *
 * \return  If task creation succeeded its id is delivered back. If the new
 *          task is active, the new task id will have a new version number so
 *          that it differs from all task ids used earlier. Chief and task
 *          number are the same as in dest task. If the new task is created
 *          inactive, the chief is taken from the chief parameter; the task
 *          number remains unchanged. The version is undefined so that the new
 *          task id might be identical with a formerly (but not currently and
 *          not in future) valid task id. This is safe since communication with
 *          inactive tasks is impossible. If task creation failed #L4_NIL_ID
 *          is returned.
 *
 * The l4_task_new() system call deletes and/or creates a task. Deletion
 * of a task means that the address space of the task and all threads of
 * the task disappear. The cpu time of all deleted threads is added to
 * the cputime of the deleting thread. If the deleted task was chief of a
 * clan, all tasks of the clan are deleted as well.
 *
 * Tasks may be created as active or inactive. For an active task, a new
 * address space is created together with 128 threads. Lthread 0 is
 * started, the other ones wait for a "real" creation using
 * l4_thread_ex_regs(). An inactive task is empty. It occupies no
 * resources, has no address space and no threads. Communication with
 * inactive tasks is not possible.
 *
 * A newly created task gets the creator as its chief, i.e. it is created
 * inside the creator's clan. Symmetrically, a task can only be deleted
 * either directly by its chief or indirectly by a higher-level chief.
 */
L4_INLINE l4_taskid_t
l4_task_new(l4_taskid_t destination,
	    l4_umword_t mcp_or_new_chief_and_flags,
	    l4_umword_t esp,
	    l4_umword_t eip,
	    l4_threadid_t pager);

/**
 * Create or delete a task, extended version.
 * \ingroup api_calls_other
 *
 * \param destination    See l4_task_new description.
 * \param mcp_or_new_chief_and_flags
 *                       See l4_task_new description.
 * \param esp            See l4_task_new description.
 * \param eip            See l4_task_new description.
 * \param pager          See l4_task_new description.
 * \param cap_handler    Set the capability fault handler for this task
 *                       and starts the task in monitored mode. The task
 *                       is only allowed to communicate with its creator
 *                       and itself (+NIL). Every other IPC will raise a
 *                       capability fault that is sent to the cap_handler
 *                       thread.
 * \param kquota         Set kernel quota for new task.
 * \param utcb           UTCB pointer of the caller.
 * \see l4_task_new
 */

L4_INLINE l4_taskid_t
l4_task_new_long(l4_taskid_t destination,
	         l4_umword_t mcp_or_new_chief_and_flags,
	         l4_umword_t esp,
	         l4_umword_t eip,
	         l4_threadid_t pager,
	         l4_threadid_t cap_handler,
	         l4_quota_desc_t kquota,
                 l4_utcb_t *utcb);


/**
 * Return pointer to Kernel Interface Page
 * \ingroup api_calls_other
 */
L4_INLINE void *
l4_kernel_interface(void);

L4_INLINE int
l4_privctrl(l4_umword_t cmd,
	    l4_umword_t param);



L4_INLINE void
l4_thread_ex_regs_sc(l4_umword_t val0,
                     l4_umword_t ip,
                     l4_umword_t sp,
                     l4_threadid_t *preempter,
                     l4_threadid_t *pager,
                     l4_umword_t *old_eflags,
                     l4_umword_t *old_eip,
                     l4_umword_t *old_esp);

L4_INLINE l4_taskid_t
l4_task_new_sc(l4_taskid_t destination,
               l4_umword_t mcp_or_new_chief_and_flags,
               l4_umword_t sp,
               l4_umword_t ip,
               l4_threadid_t pager);


/*****************************************************************************
 *** Implementation
 *****************************************************************************/

#if 1
//#ifndef CONFIG_L4_CALL_SYSCALLS

# define L4_SYSCALL_id_nearest            "int $0x31 \n\t"
# define L4_SYSCALL_fpage_unmap           "int $0x32 \n\t"
# define L4_SYSCALL_thread_switch         "int $0x33 \n\t"
# define L4_SYSCALL_thread_schedule       "int $0x34 \n\t"
# define L4_SYSCALL_lthread_ex_regs       "int $0x35 \n\t"
# define L4_SYSCALL_task_new              "int $0x36 \n\t"
# define L4_SYSCALL_privctrl              "int $0x37 \n\t"
# define L4_SYSCALL(name)                 L4_SYSCALL_ ## name

#else

# ifdef CONFIG_L4_ABS_SYSCALLS
#  define L4_SYSCALL(s) "call __l4sys_"#s"_direct@plt  \n\t"
# else
#  define L4_SYSCALL(s) "call *__l4sys_"#s"  \n\t"
# endif

#endif

#define GCC_VERSION	(__GNUC__ * 100 + __GNUC_MINOR__)

#ifdef PROFILE
#include "syscalls-l42-profile.h"
#else
#  if GCC_VERSION < 303
#    error gcc >= 3.3 required
#  else
#    include "syscalls-l42-gcc3.h"
#  endif
L4_INLINE l4_threadid_t
l4_myself_noprof(void)
{
    return l4_myself();
}

#include <l4/sys/syscalls_gen.h>

/* ============================================================= */

#endif /* ! PROFILE */

L4_INLINE void *
l4_kernel_interface(void)
{
  void *ret;
  asm (" lock; nop " : "=a"(ret) );
  return ret;
}

#endif /* !__L4_SYSCALLS_H__ */
