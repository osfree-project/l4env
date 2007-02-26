/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4sys/include/timeout.h
 * \brief   L4 absolute timeouts bits
 * \ingroup api_types
 */
/*****************************************************************************/
#ifndef __L4_SYS__TIMEOUT_H__ 
#define __L4_SYS__TIMEOUT_H__

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>

/**
 * Timeout validities
 * \ingroup api_calls_fiasco
 *
 * times are actually 2^x values (e.g. 2ms -> 2048µs)
 */ 
enum l4_timeout_abs_validity {
  L4_TIMEOUT_ABS_NOTIMEOUT = 0,
  L4_TIMEOUT_ABS_V256_us   = 0,
  L4_TIMEOUT_ABS_V512_us,
  L4_TIMEOUT_ABS_V1_ms,
  L4_TIMEOUT_ABS_V2_ms,
  L4_TIMEOUT_ABS_V4_ms,
  L4_TIMEOUT_ABS_V8_ms,    /* 5 */
  L4_TIMEOUT_ABS_V16_ms,
  L4_TIMEOUT_ABS_V32_ms,
  L4_TIMEOUT_ABS_V64_ms,
  L4_TIMEOUT_ABS_V128_ms,
  L4_TIMEOUT_ABS_V256_ms,  /* 10 */
  L4_TIMEOUT_ABS_V512_ms,
  L4_TIMEOUT_ABS_V1_s,
  L4_TIMEOUT_ABS_V2_s,
  L4_TIMEOUT_ABS_V4_s,
  L4_TIMEOUT_ABS_NEVER,    /* 15 */
};

/**
 * Timeout types.
 * \ingroup api_calls_fiasco
 */ 
enum l4_timeout_abs_type {
  L4_TIMEOUT_ABS_SEND,
  L4_TIMEOUT_ABS_RECV,
};

/* Internal use only */
enum {
  L4_TO_ABS_RECV_MODE_SHIFT     = 0,
  L4_TO_ABS_SEND_MODE_SHIFT     = 1,
  L4_TO_ABS_RECV_CLOCKBIT_SHIFT = 2,
  L4_TO_ABS_SEND_CLOCKBIT_SHIFT = 3,

  L4_TO_ABS_RECV_MODE_MASK      = 1 << L4_TO_ABS_RECV_MODE_SHIFT,
  L4_TO_ABS_SEND_MODE_MASK      = 1 << L4_TO_ABS_SEND_MODE_SHIFT,
  L4_TO_ABS_RECV_CLOCKBIT_MASK  = 1 << L4_TO_ABS_RECV_CLOCKBIT_SHIFT,
  L4_TO_ABS_SEND_CLOCKBIT_MASK  = 1 << L4_TO_ABS_SEND_CLOCKBIT_SHIFT,
};

/**
 * Calculate absolute timeouts.
 * \ingroup api_calls_fiasco
 *
 * \param id    ID used for timeout.
 * \param pint	Point in time. Clock value in the future. Usually
 *               kip->clock + delta
 * \param type  Timeout type (send or receive)
 * \param v     Timeout validity.
 *
 * \retval to	Calculated timeout.
 *
 * For timeout == never: use NEVER in validity arguments.
 * For no timeout:       use pint = 0 and validity = NOTIMEOUT
 *
 * Note: send/receive bits are not cleared but clockbits are, i.e. you can
 *       update the timeout value but you should not mix this timeout for
 *       sending and receiving without clearing this yourself.
 */
L4_INLINE
void l4_timeout_abs(l4_threadid_t *id,
                    l4_kernel_clock_t pint,
                    enum l4_timeout_abs_type type,
                    enum l4_timeout_abs_validity v,
                    l4_timeout_t *to);

L4_INLINE
void l4_timeout_abs(l4_threadid_t *id,
                    l4_kernel_clock_t pint,
                    enum l4_timeout_abs_type type,
                    enum l4_timeout_abs_validity v,
                    l4_timeout_t *to)
{
  int e, m, c;

  e = 15 - v;
  m = pint >> v;
  c = m >> 8 & 1;
  m &= 0xff;
  
  if (type == L4_TIMEOUT_ABS_SEND)
    {
      to->to.snd_exp = e;
      to->to.snd_man = m;
      id->id.chief   |= L4_TO_ABS_SEND_MODE_MASK;
      if (c)
        id->id.chief |=  L4_TO_ABS_SEND_CLOCKBIT_MASK;
      else
        id->id.chief &= ~L4_TO_ABS_SEND_CLOCKBIT_MASK;
    }
  else
    {
      to->to.rcv_exp = e;
      to->to.rcv_man = m;
      id->id.chief   |= L4_TO_ABS_RECV_MODE_MASK;
      if (c)
        id->id.chief |=  L4_TO_ABS_RECV_CLOCKBIT_MASK;
      else
        id->id.chief &= ~L4_TO_ABS_RECV_CLOCKBIT_MASK;
    }
}

#endif /* !__L4_SYS__TIMEOUT_H__ */
