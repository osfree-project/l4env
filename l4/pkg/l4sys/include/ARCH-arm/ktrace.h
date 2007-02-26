/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4sys/includes/ARCH-x86/L4API-l4v2/ktrace.h
 * \brief   L4 kernel event tracing
 * \ingroup api_calls
 */
/*****************************************************************************/
#ifndef __L4_KTRACE_H__
#define __L4_KTRACE_H__

#include <l4/sys/types.h>
#include <l4/sys/kdebug.h>

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

#define LOG_EVENT_MAX_EVENTS      16  /**< Maximum number of events
				       **  \ingroup api_calls_fiasco
				       **/

/**
 * Tracebuffer status.
 * \ingroup api_calls_fiasco
 */
typedef struct
{
  /// Address of tracebuffer 0
  l4_umword_t tracebuffer0;
  /// Size of tracebuffer 0
  l4_umword_t size0;
  /// Version number of tracebuffer 0 (incremented if tb0 overruns)
  l4_umword_t version0;
  /// Address of tracebuffer 1 (there is no gap between tb0 and tb1)
  l4_umword_t tracebuffer1;
  /// Size of tracebuffer 1 (same as tb0)
  l4_umword_t size1;
  /// Version number of tracebuffer 1 (incremented if tb1 overruns)
  l4_umword_t version1;
  /// Available LOG events
  l4_umword_t logevents[LOG_EVENT_MAX_EVENTS];

  /// Scaler used for translation of CPU cycles to nano seconds
  l4_umword_t scaler_tsc_to_ns;
  /// Scaler used for translation of CPU cycles to micro seconds
  l4_umword_t scaler_tsc_to_us;
  /// Scaler used for translation of nano seconds to CPU cycles
  l4_umword_t scaler_ns_to_tsc;

  /// Number of context switches (intra AS or inter AS)
  l4_umword_t cnt_context_switch;
  /// Number of inter AS context switches
  l4_umword_t cnt_addr_space_switch;
  /// How often was the IPC shortcut taken
  l4_umword_t cnt_shortcut_failed;
  /// How often was the IPC shortcut not taken
  l4_umword_t cnt_shortcut_success;
  /// Number of hardware interrupts (without kernel scheduling interrupt)
  l4_umword_t cnt_irq;
  /// Number of long IPCs
  l4_umword_t cnt_ipc_long;

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

/*****************************************************************************
 *** Implementation
 *****************************************************************************/

L4_INLINE l4_tracebuffer_status_t *
fiasco_tbuf_get_status(void)
{
  /* Not implemented */
  return (l4_tracebuffer_status_t *)NULL;
}

L4_INLINE l4_umword_t
fiasco_tbuf_log(const char *text)
{
  /* Not implemented */
  return 0;
}

L4_INLINE l4_umword_t
fiasco_tbuf_log_3val(const char *text, unsigned v1, unsigned v2, unsigned v3)
{
  /* Not implemented */
  return 0;
}

L4_INLINE void
fiasco_tbuf_clear(void)
{
  /* Not implemented */
}

L4_INLINE void
fiasco_tbuf_dump(void)
{
  /* Not implemented */
}

#endif

