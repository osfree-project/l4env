/*!
 * \file   sys/include/ARCH-x86/rt_sched-proto.h
 * \brief  Identifier and prototype definitions for real-time scheduling
 *
 * \date   01/05/2005
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __SYS_INCLUDE_ARCH_X86_RT_SCHED_PROTO_H_
#define __SYS_INCLUDE_ARCH_X86_RT_SCHED_PROTO_H_

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>

#define L4_RT_ADD_TIMESLICE	1
#define L4_RT_REM_TIMESLICES	2
#define L4_RT_SET_PERIOD	3
#define L4_RT_BEGIN_PERIODIC	4
#define L4_RT_BEGIN_PERIODIC_NS	5
#define L4_RT_END_PERIODIC	6
#define L4_RT_CHANGE_TIMESLICE	7

#define L4_RT_NEXT_PERIOD	0x10
#define L4_RT_PREEMPTION_ID	0x20

typedef struct {
    l4_uint64_t time:56;
    l4_uint64_t id:7;
    l4_uint64_t type:1;
} l4_rt_preemption_val_t;
typedef struct {
    l4_uint32_t time_high:24;
    l4_uint32_t id:7;
    l4_uint32_t type:1;
} l4_rt_preemption_val32_t;
typedef union {
    l4_low_high_t lh;
    l4_rt_preemption_val_t p;
} l4_rt_preemption_t;

 /* premmption ipc types: values of the type field */
#define L4_RT_PREEMPT_DEADLINE	0	/* deadline miss */
#define L4_RT_PREEMPT_TIMESLICE	1	 /* timeslice overrun */

/*!\brief Add a timeslice for periodic execution.
 * \ingroup api_calls_rt_sched
 *
 * \param  dest		thread to add the timeslice to
 * \param  prio		priority of the timeslice
 * \param  time		length of the timeslice in microseconds
 * \retval 0		OK
 * \retval -1		Error, one of:
 *			- dest does not exist
 *			- insufficient MCP (old or new prio>MCP),
 *			- dest running in periodic mode or transitioning to
 *			- time quantum 0 or infinite
 */
L4_INLINE int
l4_rt_add_timeslice(l4_threadid_t dest, int prio, int time);

/*!\brief Change a timeslice for periodic execution.
 * \ingroup api_calls_rt_sched
 *
 * \param  dest		thread whose timing parameters are to change
 * \param  id		number of the time-slice to change (rt start at 1)
 * \param  prio		new priority of the timeslice
 * \param  time		new length of the timeslice in microseconds,
 *			0: don't change.
 * \retval 0		OK
 * \retval -1		Error, one of:
 *			- dest does not exist
 *			- insufficient MCP (old or new prio>MCP),
 *			- timeslice does not exist
 *
 * This function modifies the priority and optionally the length of an
 * existing timeslice of a thread. When calling this function while
 * the timeslice is active, the effect may be delayed till the next
 * period.
 *
 * This function can be called as soon as the denoted timeslice was
 * added with l4_rt_add_timeslice(). Thus, the thread may have started
 * periodic execution already, but it needs not.
 */
L4_INLINE int
l4_rt_change_timeslice(l4_threadid_t dest, int id, int prio, int time);

/*!\brief Start strictly periodic execution
 * \ingroup api_calls_rt_sched
 *
 * \param  dest		thread that starts periodic execution
 * \param  clock	absolute time to start.
 * \retval 0		OK
 * \retval -1		Error, one of:
 *			- dest does not exist
 *			- insufficient MCP (old or new prio>MCP),
 *			- dest running in periodic mode or transitioning to
 *
 * Call this function to start the periodic execution after setting up
 * the timeslices using l4_rt_add_timeslice() and l4_rt_set_period(). 
 *
 * By the time specified in clock thread dest must wait for the next period,
 * e.g. by using l4_rt_next_period() or some other IPC with the
 * L4_RT_NEXT_PERIOD flag enabled. Otherwise the transition to periodic mode
 * fails.
 */
L4_INLINE int
l4_rt_begin_strictly_periodic(l4_threadid_t dest, l4_kernel_clock_t clock);

/*!\brief Start periodic execution with minimal interrelease times
 * \ingroup api_calls_rt_sched
 *
 * \param  dest		thread that starts periodic execution
 * \param  clock	absolute time to start.
 * \retval 0		OK
 * \retval -1		Error, one of:
 *			- dest does not exist
 *			- insufficient MCP (old or new prio>MCP),
 *			- dest running in periodic mode or transitioning to
 *
 * Call this function to start the periodic execution after setting up
 * the timeslices using l4_rt_add_timeslice() and l4_rt_set_period().
 *
 * By the time specified in clock thread dest must wait for the next period,
 * e.g. by using l4_rt_next_period() or some other IPC with the
 * L4_RT_NEXT_PERIOD flag enabled. Otherwise the transition to periodic mode
 * fails.
 */
L4_INLINE int
l4_rt_begin_minimal_periodic(l4_threadid_t dest, l4_kernel_clock_t clock);

/*!\brief Stop periodic execution
 * \ingroup api_calls_rt_sched
 *
 * \param  dest		thread that stops periodic execution
 * \retval 0		OK
 * \retval -1		Error, one of:
 *			- dest does not exist
 *			- insufficient MCP (old or new prio>MCP),
 *			- dest not running in periodic mode and
 *			  not transitioning to
 *
 * This function aborts the periodic execution of thread dest. Thread dest
 * returns to conventional scheduling then.
 */
L4_INLINE int
l4_rt_end_periodic(l4_threadid_t dest);

/*!\brief   Remove all reservation scheduling contexts
 * \ingroup api_calls_rt_sched
 *
 * This function removes all the scheduling contexts that were set up
 * so far for the given thread.
 *
 * \param  dest		thread the scheduling contexts should be removed from
 * \retval 0		OK
 * \retval -1		Error, one of:
 *			- dest does not exist
 *			- insufficient MCP
 *			- dest running in periodic mode or transitioning to
 */
L4_INLINE int
l4_rt_remove(l4_threadid_t dest);

/*!\brief Set the length of the period
 * \ingroup api_calls_rt_sched
 *
 * This function sets the length of the period for periodic execution.
 *
 * \param  dest		destination thread
 * \param  clock	period length in microsendonds. Will be rounded up
 *			by the kernel according to the timer granularity.
 * \return		This function always succeeds.
 */
L4_INLINE void
l4_rt_set_period(l4_threadid_t dest, l4_kernel_clock_t clock);

/*!\brief activate the next timeslice (scheduling context)
 * \ingroup api_calls_rt_sched
 *
 * \param  id		The ID of the timeslice we think we are on
 *			(current timeslice)
 * \param  clock	pointer to a l4_kernel_clock_t variable
 * \retval 0		OK, *clock contains the remaining time of the timeslice
 * \retval -1		Error, id did not match current timeslice
 *
 */
L4_INLINE int
l4_rt_next_reservation(unsigned id, l4_kernel_clock_t*clock);

/*!\brief Wait for the next period, skipping all unused timeslices
 * \ingroup api_calls_rt_sched
 *
 * \retval 0		OK
 * \retval !0		IPC Error.
 *
 */
L4_INLINE int
l4_rt_next_period(void);

/*!\brief Return the preemption id of a thread
 * \ingroup api_calls_rt_sched
 *
 * \param  id		thread
 *
 * \return thread-id of the (virtual) preemption IPC sender
 */
L4_INLINE l4_threadid_t
l4_preemption_id(l4_threadid_t id);

/*!\brief Return thread-id that flags waiting for the next period
 * \ingroup api_calls_rt_sched
 *
 * \param  id		original thread-id
 *
 * \return modified id, to be used in an IPC, waiting for the next period.
 */
L4_INLINE l4_threadid_t
l4_next_period_id(l4_threadid_t id);

/*!\brief Generic real-time setup function
 * \ingroup api_calls_rt_sched
 *
 * \param  dest		destination thread
 * \param  param	scheduling parameter
 * \param  clock	clock parameter
 * \retval 0		OK
 * \retval -1		Error.
 *
 * This function is not meant to be used directly, it is merely used
 * by others.
 */
L4_INLINE int
l4_rt_generic(l4_threadid_t dest, l4_sched_param_t param,
		    l4_kernel_clock_t clock);

/*!\brief Delayed preemption: Reserve a duration
 *
 * \param  duration	time (microseconds)
 * \retval 0		OK
 * \retval !0		Error.
 *
 * This function reserves a non-preemptible duration at the kernel.
 *
 * \note Due to the currently missing support in the kernel, this
 *       call is a no-op.
 */
L4_INLINE int
l4_rt_dp_reserve(int duration);

/*!\brief Delayed preemption: Cancel a reservation
 *
 * \retval 0		OK
 * \retval !0		Error.
 */
L4_INLINE void
l4_rt_dp_remove(void);

/*!\brief Delayed preemption: Start a delayed preemption
 *
 * After making a dp reservation, a thread can start an uninterruptible
 * execution for the reserved time.
 *
 * \note Due to the currently missing support in the kernel, this
 *       call is implemented as a cli(), requiring cooperation from the
 *	 calling thread.
 */
L4_INLINE void
l4_rt_dp_begin(void);

/*!\brief Delayed preemption: End a delayed preemption
 *
 * After calling this function, the thread may be preempted by the kernel
 * again.
 */
L4_INLINE void
l4_rt_dp_end(void);

/***************************************************************************
 *** IMPLEMENTATIONS
 ***************************************************************************/

L4_INLINE l4_threadid_t
l4_preemption_id(l4_threadid_t id)
{
  //id.id.chief |= L4_RT_PREEMPTION_ID;
  return id;
}

L4_INLINE l4_threadid_t
l4_next_period_id(l4_threadid_t id)
{
  //id.id.chief |= L4_RT_NEXT_PERIOD;
  return id;
}

#endif /* ! __L4_SYS__RT_SCHED_H__ */
