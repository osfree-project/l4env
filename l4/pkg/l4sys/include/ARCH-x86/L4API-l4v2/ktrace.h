/* $Id$ */
/*****************************************************************************/
/*!
 * \file    l4sys/include/ARCH-x86/L4API-l4v2/ktrace.h
 * \brief   L4 kernel event tracing
 * \ingroup api_calls
 */
/*****************************************************************************/
#ifndef __L4_KTRACE_H__
#define __L4_KTRACE_H__

#include <l4/sys/types.h>
#include <l4/sys/ktrace_events.h>

#define LOG_EVENT_CONTEXT_SWITCH   0  /**< Event: context switch
				       **  \ingroup api_calls_fiasco
				       **/
#define LOG_EVENT_IPC_SHORTCUT     1  /**< Event: IPC shortcut
				       **  \ingroup api_calls_fiasco
				       **/
#define LOG_EVENT_IRQ_RAISED       2  /**< Event: IRQ occurred
				       **  \ingroup api_calls_fiasco
				       **/
#define LOG_EVENT_TIMER_IRQ        3  /**< Event: Timer IRQ occurred
				       **  \ingroup api_calls_fiasco
				       **/
#define LOG_EVENT_THREAD_EX_REGS   4  /**< Event: thread_ex_regs
				       **  \ingroup api_calls_fiasco
				       **/
#define LOG_EVENT_TRAP             5  /**< Event: Trap occured
				       **  \ingroup api_calls_fiasco
				       **/
#define LOG_EVENT_PF_RES           6  /**< Event: Pagefpault resolved
				       **  \ingroup api_calls_fiasco
				       **/
#define LOG_EVENT_SCHED            7  /**< Event: Scheduling context
				       **  loaded, saved or invalidated
				       **  \ingroup api_calls_fiasco
				       **/
#define LOG_EVENT_PREEMPTION       8  /**< Event: Preemption IPC sent
				       **  \ingroup api_calls_fiasco
				       **/
#define LOG_EVENT_ID_NEAREST       9  /**< Event: ID nearest
				       **  \ingroup api_calls_fiasco
				       **/
#define LOG_EVENT_TASK_NEW        10  /**< Event: New task created
				       **  \ingroup api_calls_fiasco
				       **/

#define LOG_EVENT_MAX_EVENTS      16  /**< Maximum number of events
				       **  \ingroup api_calls_fiasco
				       **/

/**
 * Tracebuffer status.
 * \ingroup api_calls_fiasco
 */
// keep in sync with fiasco/src/jabi/jdb_ktrace.cpp
typedef struct
{
  /// Address of tracebuffer 0
  l4_tracebuffer_entry_t * tracebuffer0;
  /// Size of tracebuffer 0
  l4_umword_t size0;
  /// Version number of tracebuffer 0 (incremented if tb0 overruns)
  volatile l4_uint64_t version0;
  /// Address of tracebuffer 1 (there is no gap between tb0 and tb1)
  l4_tracebuffer_entry_t * tracebuffer1;
  /// Size of tracebuffer 1 (same as tb0)
  l4_umword_t size1;
  /// Version number of tracebuffer 1 (incremented if tb1 overruns)
  volatile l4_uint64_t version1;
  /// Address of the most current event in tracebuffer.
  volatile l4_tracebuffer_entry_t * current_entry;
  /// Available LOG events
  l4_umword_t logevents[LOG_EVENT_MAX_EVENTS];

  /// Scaler used for translation of CPU cycles to nano seconds
  l4_umword_t scaler_tsc_to_ns;
  /// Scaler used for translation of CPU cycles to micro seconds
  l4_umword_t scaler_tsc_to_us;
  /// Scaler used for translation of nano seconds to CPU cycles
  l4_umword_t scaler_ns_to_tsc;

  /// Number of context switches (intra AS or inter AS)
  volatile l4_umword_t cnt_context_switch;
  /// Number of inter AS context switches
  volatile l4_umword_t cnt_addr_space_switch;
  /// How often was the IPC shortcut taken
  volatile l4_umword_t cnt_shortcut_failed;
  /// How often was the IPC shortcut not taken
  volatile l4_umword_t cnt_shortcut_success;
  /// Number of hardware interrupts (without kernel scheduling interrupt)
  volatile l4_umword_t cnt_irq;
  /// Number of long IPCs
  volatile l4_umword_t cnt_ipc_long;
  /// Number of page faults
  volatile l4_umword_t cnt_page_fault;
  /// Number of faults (application runs at IOPL 0 and tries to execute
  /// cli, sti, in, or out but does not have a sufficient in the I/O bitmap)
  volatile l4_umword_t cnt_io_fault;
  /// Number of tasks created
  volatile l4_umword_t cnt_task_create;
  /// Number of reschedules
  volatile l4_umword_t cnt_schedule;
  /// Number of flushes of the I/O bitmap. Increases on context switches
  /// between two small address spaces if at least one of the spaces has
  /// an I/O bitmap allocated.
  volatile l4_umword_t cnt_iobmap_tlb_flush;

} l4_tracebuffer_status_t;

/**
 * Return tracebuffer status.
 * \ingroup api_calls_fiasco
 *
 * \return Pointer to tracebuffer status struct.
 */
L4_INLINE l4_tracebuffer_status_t *
fiasco_tbuf_get_status(void);

/**
 * Return the physical address of the tracebuffer status struct.
 * \ingroup api_calls_fiasco
 *
 * \return physical address of status struct.
 */
L4_INLINE l4_addr_t
fiasco_tbuf_get_status_phys(void);

/**
 * Create new tracebuffer entry with describing \<text\>.
 * \ingroup api_calls_fiasco
 *
 * \param  text   Logging text
 * \return Pointer to tracebuffer entry
 */
L4_INLINE l4_umword_t
fiasco_tbuf_log(const char *text);

/**
 * Create new tracebuffer entry with describing \<text\> and three additional
 * values.
 * \ingroup api_calls_fiasco
 *
 * \param  text   Logging text
 * \param  v1     first value
 * \param  v2     second value
 * \param  v3     third value
 * \return Pointer to tracebuffer entry
 */
L4_INLINE l4_umword_t
fiasco_tbuf_log_3val(const char *text, unsigned v1, unsigned v2, unsigned v3);

/**
 * Clear tracebuffer.
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_tbuf_clear(void);

/**
 * Dump tracebuffer to kernel console.
 * \ingroup api_calls_fiasco
 */
L4_INLINE void
fiasco_tbuf_dump(void);

/**
 * Disable the kernel scheduling timer.
 */
L4_INLINE void
fiasco_timer_disable(void);

/**
 * Enable the kernel scheduling timer (after it was disabled with
 * fiasco_timer_disable).
 */
L4_INLINE void
fiasco_timer_enable(void);

/*****************************************************************************
 *** Implementation
 *****************************************************************************/

L4_INLINE l4_tracebuffer_status_t *
fiasco_tbuf_get_status(void)
{
  l4_tracebuffer_status_t *tbuf;
  asm("int $3; cmpb $29, %%al" : "=a" (tbuf) : "0" (0));
  return tbuf;
}

L4_INLINE l4_addr_t
fiasco_tbuf_get_status_phys(void)
{
  l4_addr_t tbuf_phys;
  asm("int $3; cmpb $29, %%al" : "=a" (tbuf_phys) : "0" (5));
  return tbuf_phys;
}

L4_INLINE l4_umword_t
fiasco_tbuf_log(const char *text)
{
  l4_umword_t offset;
  asm volatile("int $3; cmpb $29, %%al"
	      : "=a" (offset)
	      : "a" (1), "d" (text));
  return offset;
}

L4_INLINE l4_umword_t
fiasco_tbuf_log_3val(const char *text, unsigned v1, unsigned v2, unsigned v3)
{
  l4_umword_t offset;
  asm volatile("int $3; cmpb $29, %%al"
	      : "=a" (offset)
	      : "a" (4), "d" (text), "c" (v1), "S" (v2), "D" (v3));
  return offset;
}

L4_INLINE void
fiasco_tbuf_clear(void)
{
  asm volatile("int $3; cmpb $29, %%al" : : "a" (2));
}

L4_INLINE void
fiasco_tbuf_dump(void)
{
  asm volatile("int $3; cmpb $29, %%al" : : "a" (3));
}

L4_INLINE void
fiasco_timer_disable(void)
{
  asm volatile("int $3; cmpb $29, %%al" : : "a" (6));
}

L4_INLINE void
fiasco_timer_enable(void)
{
  asm volatile("int $3; cmpb $29, %%al" : : "a" (7));
}

#endif

