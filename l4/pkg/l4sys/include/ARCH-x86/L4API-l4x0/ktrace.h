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
 * Tracebuffer status
 * \ingroup api_calls_fiasco
 */
typedef struct
{
  l4_umword_t tracebuffer0;
  l4_umword_t size0;
  l4_umword_t version0;
  l4_umword_t tracebuffer1;
  l4_umword_t size1;
  l4_umword_t version1;
  l4_umword_t logevents[LOG_EVENT_MAX_EVENTS];
  l4_umword_t scaler_tsc_to_ns;
  l4_umword_t scaler_tsc_to_us;
  l4_umword_t scaler_ns_to_tsc;
} l4_tracebuffer_status_t;

/**
 * Return tracebuffer status
 * \ingroup api_calls_fiasco
 * 
 * \return Pointer to tracebuffer status struct.
 */
L4_INLINE l4_tracebuffer_status_t *
fiasco_get_tbuf_status(void);

/**
 * Create new tracebuffer entry with describing <text>.
 * \ingroup api_calls_fiasco
 *
 * \return Pointer to tracebuffer entry 
 */
L4_INLINE l4_umword_t
fiasco_tbuf_log(const char *text);

/**
 * Create new tracebuffer entry with describing <text> and three additional
 * values.
 * \ingroup api_calls_fiasco
 *
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
fiasco_get_tbuf_status(void)
{
  l4_tracebuffer_status_t *tbuf;
  asm("int $3; cmpb $29, %%al" : "=a" (tbuf) : "0" (0));
  return tbuf;
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

#endif

