/*!
 * \file   cpu_reserve/include/sched.h
 * \brief  CPU reservation prototypes
 *
 * \date   09/06/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __CPU_RESERVE_INCLUDE_SCHED_H_
#define __CPU_RESERVE_INCLUDE_SCHED_H_
#include <l4/sys/types.h>
#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

#define l4cpu_reserve_name "cpu_res"

/*!\brief Make a reservation
 * \ingroup api
 *
 * \param  thread	thread to make a reservation for
 * \param  name		a name for the reservation
 * \param  prio		priority of the reservation
 * \param  period	period of the thread
 * \param  wcet		- in: WCET of the thread per period
 *			- out: reserved amount of CPU in case of success
 * \param  deadline	deadline within the period. Specify 0 for an optional
 *			reservation.
 * \param  id		will be filled with the id of the timeslice.
 * \retval 0		OK
 * \retval !0		Error
 *
 * This function makes a reservation for the given thread and adds the
 * needed timeslices at the kernel. Open success, it is ensured that
 * the thread will receive the requested amount of CPU within the
 * given deadline. However, a general assumption is that no other,
 * higher prioritized threads exist in the system that are not
 * registered at the CPU reservation server. Use the MCP mechanism of
 * the kernel to ensure this.
 *
 * To add multiple timeslices for a thread, call this function
 * multiple times. This function sets the period of the thread when
 * adding the first timeslice to thread. So there is no additional
 * set_period() call as with the raw kernel interface.
 */
extern int l4cpu_reserve_add(l4_threadid_t thread,
			     const char*name,
			     int prio,
			     int period,
			     int *wcet,
			     int deadline,
			     int *id);

/*!\brief Add delayed preemption reservation for a thread, internal function
 * \ingroup dp
 *
 * \param  thread	thread to add delayed preemption (DP) to
 * \param  id		id of the timeslice to add DP to
 * \param  prio		sensitive priority of DP, once it is activated.
 *			(Ignored and assumed to be 255, see note.)
 * \param  delay	max. duration of DP. Will be rounded up according to
 *			kernel capabilities and returned.
 *
 * \retval 0		OK
 * \retval !0		Error
 *
 * This function adds a delayed preemption to the specified timeslice,
 * which must exist already. The non real-time timeslice with id=0
 * always exists, the others are created by calling
 * l4cpu_reserve_add(). The actual duration of the delayed preemption
 * is returned in *delay. The name returned by the diagnostic function
 * l4cpu_reserve_scheds_get() is set to the name of the timeslice,
 * suffixed by ".dp".
 *
 * The delayed preemption reservation will be removed when the
 * reservations for the thread are removed.
 *
 * \note As of today (Oct 2004), the kernel has no support for DP, thus
 *       a user-level emulation is needed. CLI/STI comes into mind, which
 *       of course ignores the sensitive priority and prevents preemption
 *       entirely as long as DP is activated.
 */
extern int l4cpu_reserve_delayed_preempt(l4_threadid_t thread,
					 int id,
					 int prio,
					 int *delay);

/*!\brief Change the reservation of a thread
 * \ingroup api
 *
 * \param  thread	thread to change the reservation for
 * \param  id		id of the timeslice to change
 * \param  new_prio	new priority, -1 does not change
 * \param  new_wcet	- in: new wcet, -1 does not change
 *			- out: reserved amount of CPU if successfully modified
 * \param  deadline	new deadline, -1 does not change.
 * \retval 0		OK
 * \retval !0		Error
 *
 * This function changes a reservation for the given thread and
 * timeslice. Checks are performed to ensure that all existing
 * reservations can be met.
 */
extern int l4cpu_reserve_change(l4_threadid_t thread,
				int id,
				int new_prio,
				int *new_wcet,
				int new_deadline);

/*!\brief Remove the reservations of a thread
 * \ingroup api
 *
 * \param  thread	thread to remove all reservations from
 * \retval 0		OK
 * \retval !0		Error
 *
 * This function removes all reservations for the given thread.
 */
extern int l4cpu_reserve_delete_thread(l4_threadid_t thread);

/*!\brief Remove the reservations of a task
 * \ingroup api
 *
 * \param  task		task to remove all reservations from
 * \retval 0		OK
 * \retval !0		Error
 *
 * This function removes all reservations for all threads of the given 
 * task.
 */
extern int l4cpu_reserve_delete_task(l4_threadid_t task);

/*!\brief Start strictly periodic execution
 * \ingroup api
 *
 * \param  dest		thread that starts periodic execution
 * \param  clock	absolute time to start.
 * \retval 0		OK
 * \retval !0		Error
 *
 * Call this function to start the periodic execution of another thread after
 * setting up the reservations using l4cpu_reserve_add().
 * 
 * For details, see l4_rt_begin_strictly_periodic() in the l4sys package.
 *
 * \note Starting RT-mode of the own thread with a timeout is unsafe. As
 *       the thread has no reservation, a deadline for activating this call
 *	 at the kernel can not be guaranteed. Starting the own thread requires
 *	 the NP bit to be set when calling the scheduler to activate RT mode
 *	 to ensure the thread is ready to receive its first NP-IPC at the
 *	 right time. If, however, the RT-mode activation is triggered after
 *	 the timeout, the operation fails. But, the thread is waiting for
 *       the NP-IPC and won't receive the answer indicating the error.
 */
extern int l4cpu_reserve_begin_strictly_periodic(l4_threadid_t thread,
						 l4_kernel_clock_t clock);

/*!\brief Start strictly periodic execution of own thread ASAP
 * \ingroup api
 *
 * \param  me		(cached) own ID. L4_INVALID_ID if you don't know.
 * \retval 0		OK
 * \retval !0		Error
 *
 * Call this function to start the periodic execution for the calling
 * thread after setting up the reservations using l4cpu_reserve_add().
 * The initial next_period() call is automatically executed, and periodic
 * mode is activated as sonn as possible.
 * 
 * For details, see l4_rt_begin_strictly_periodic() in the l4sys package.
 */
extern int l4cpu_reserve_begin_strictly_periodic_self(l4_threadid_t me);

/*!\brief Deprecated version with timeout, which is unsafe */
extern int l4cpu_reserve_begin_strictly_periodic_self_deprecated(
    l4_threadid_t me, l4_kernel_clock_t clock);

/*!\brief Start periodic execution with minimal interrelease times
 * \ingroup api
 *
 * \param  dest		thread that starts periodic execution
 * \param  clock	absolute time to start.
 * \retval 0		OK
 * \retval !0		Error
 *
 * Call this function to start the periodic execution after setting up
 * the reservations using l4cpu_reserve_add().
 * 
 * For details, see l4_rt_begin_minimal_periodic() in the l4sys package.
 */
extern int l4cpu_reserve_begin_minimal_periodic(l4_threadid_t thread,
						l4_kernel_clock_t clock);

/*!\brief Start periodic execution with minimal interrelease times, own thread
 * \ingroup api
 *
 * \param  me		(cached) own ID. L4_INVALID_ID if you don't know.
 * \param  clock	absolute time to start. 0: ASAP.
 * \retval 0		OK
 * \retval !0		Error
 *
 * Call this function to start the periodic execution for the calling
 * thread after setting up the reservations using l4cpu_reserve_add().
 * The initial next_period() call is automatically executed.
 * 
 * For details, see l4_rt_begin_minimal_periodic() in the l4sys package.
 * For notes, see l4cpu_reserve_begin_strictly_periodic_self().
 */
extern int l4cpu_reserve_begin_minimal_periodic_self(l4_threadid_t thread);

extern int l4cpu_reserve_begin_minimal_periodic_self_deprecated(
    l4_threadid_t thread, l4_kernel_clock_t clock);

/*!\brief Stop periodic execution
 * \ingroup api
 *
 * \param  thread	thread that stops periodic execution
 * \retval 0		OK
 * \retval !0		Error
 *
 * This function aborts the periodic execution of the given thread. 
 *
 * For details, see l4_rt_end_periodic() in the l4sys package.
 */
extern int l4cpu_reserve_end_periodic(l4_threadid_t thread);

/*!\brief Watch exception IPCs for the given thread
 * \ingroup api
 *
 * \param  thread	thread to be watched
 * \param  addr		will contain ptr to overflow-field after return.
 *			if 0, no address will be provided in return.
 *
 * The function starts watching for exception IPCs (deadline miss or
 * timeslice expired) at cpu_reserved. If #addr is not 0 on calling,
 * the returned array contains an unsigned for each timeslice id, which
 * is incremented on each timeslice expired IPC.
 */
extern int l4cpu_reserve_watch(l4_threadid_t thread,
			       unsigned **addr);

/*!\brief Return the number of reservations made.
 * \ingroup api
 *
 * Use this function for debugging/monitoring purposes.
 */
extern int l4cpu_reserve_scheds_count(void);

/*!\brief Return a given reservation.
 * \ingroup api
 *
 * \param idx		reservation index, 0<=idx<=l4cpu_reserve_scheds_count()
 * \param name		will set to the name, free() it after use
 * \param thread	thread-id of reservation
 * \param creator	thread-id of the creator of the reservation
 * \param id		id of the reservation
 * \param prio		priority of the reservation
 * \param period	period of the reservation
 * \param wcet		WCET of the reservation
 * \param deadline	deadline of the reservation, 0 means optional
 *
 * Use this function for debugging/monitoring purposes.
 */
extern int l4cpu_reserve_scheds_get(int idx,
				    char **name,
				    l4_threadid_t *thread,
				    l4_threadid_t *creator,
				    int *id,
				    int *prio,
				    int *period,
				    int *wcet,
				    int *deadline);


/*!\brief Return response time of given thread
 *
 * \param thread	thread id
 * \param id		timeslice id
 *
 * \retval >=0		worst-case response time according to given
 *			reservations
 * \retval -L4_ETIME	response-time larger than period (or deadline, if
 *			set) of the thread
 * \retval -L4_EINVAL	thread/timeslice not registered.
 *
 */
extern int l4cpu_reserve_time_demand(l4_threadid_t thread,
				     int id);


/*!\brief Delayed Preemption: Reserve a duration for the whole task
 * \ingroup dp
 * 
 * \param  duration	time (microseconds)
 * \retval duration	contains the actually reserved time.
 * \retval 0		OK
 * \retval !0		Error.
 *
 * This function reserves a non-preemptible duration at the kernel, for
 * all existing and future threads of the calling task.
 *
 * \note The task-abstraction is easy to use from client applications,
 * as nothing has to be done on thread creation. It is easy to
 * implement with the current scheme too. It is also correct in the
 * sense of reservation and scheduling analysis, as long as the
 * priority of the thread reserving the delayed preemption is _low_
 * enough. Keep in mind that only one blocking may happen at a time,
 * and therefore the maximum blocking time of higher-prioritized
 * threads is determined by the maximum the DP times of all
 * lower-prioritized threads.
 *
 * \note With future kernel releases, that might include real kernel
 * support for DP, the task abstraction might be hard to implement,
 * and support for DP must be programmed more explicitely. Think of
 * interrupt threads that are created by DDE internally.
 */
extern int l4cpu_dp_reserve_task(int *duration);

/*!\brief Delayed preemption: Cancel a reservation
 * \ingroup dp
 *
 * \retval 0		OK
 * \retval !0		Error.
 *
 * \note Currently unimplemented.
 */
extern int l4cpu_dp_remove(void);

extern void*l4cpu_dp_pc;
/*!\brief Return if DP is currently running. */
extern int l4cpu_dp_active(void);

/*!\brief Callback for start of DP section
 *
 * This function is called if an delayed preemption actually starts.
 */
extern void l4cpu_dp_start_callback(void) __attribute__((weak));

/*!\brief Delayed preemption: Start a delayed preemption
 * \ingroup dp
 *
 * After making a dp reservation with l4cpu_dp_reserve_task(), a
 * thread can start an uninterruptible execution. Calls to this function
 * may be nested, and the last call to l4cpu_dp_end() actually leaves the
 * uninterruptible section.
 *
 * Nesting calls from different threads will be detected and interpreted
 * as an error. Kernel debugger is entered.
 *
 * \note Due to the currently missing support in the kernel, this
 *       call is implemented as a cli(), requiring cooperation from the
 *	 calling thread.
 *
 * \retval 0		OK
 * \retval !0		Error, e.g. nesting calls from different threads.
 *
 * \pre l4_tsc_init() must have been called successfully.
 */
extern int l4cpu_dp_begin(void);

/*!\brief Callback for end of DP section
 *
 * This function is called if an delayed preemption actually ends, this is
 * when all nested start calls have their stop calls.
 *
 * \param diff		the time the delayed preemption was active
 * \param eip		the eip that started the delayed preemption
 */
extern void l4cpu_dp_stop_callback(unsigned diff, void*eip)
    __attribute__((weak));

/*!\brief Delayed preemption: End a delayed preemption
 * \ingroup dp
 *
 * When calling this function, and un-nesting all previous calls to
 * l4cpu_dp_begin(), the thread may be preempted by the kernel again.
 *
 * \note In its current implementation, time measurement is used to verify
 *       the actual blocking time. A warning is printed if the blocking
 *       was too long.
 *
 * \retval 0		OK. probably someone still in the preemption
 * \retval >0		OK, DP ended. Time left (in usecs).
 * \retval -L4_ETIME	blocking was too long
 * \retval -L4_EINVAL	DP was not running
 *
 * \pre l4_tsc_init() must have been called successfully.
 */
extern int l4cpu_dp_end(void);

/*!\brief Get the reservations at the reservation server and dump them
 */
extern int l4cpu_reserve_dump(void);

int l4cpu_reserve_wait_periodic(l4_threadid_t parent);
int l4cpu_reserve_wait_periodic_ready(int mode,
				      l4_threadid_t child,
				      l4_kernel_clock_t clock);

EXTERN_C_END
#endif
