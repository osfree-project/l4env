/*!
 * \file    l4sys/include/__timeout.h
 * \brief   Timeout definitions
 * \ingroup api_calls
 */
#ifndef L4_SYS_TIMEOUT_H__
#define L4_SYS_TIMEOUT_H__

#include <l4/sys/l4int.h>

/**
 * Basic timeout specification.
 *
 * Basically a floating point number with 10 bits mantissa and
 * 5 bits exponent (t = m*2^e).
 *
 * The timout can also specify an absolute point in time (bit 16 == 1).
 */
typedef struct l4_timeout_s {
  l4_uint16_t t;
} __attribute__((packed)) l4_timeout_s;


/**
 * For IPC there are usually a send and a receive timeout. So here is this 
 * pair.
 */
typedef union l4_timeout_t {
  l4_uint32_t raw;
  struct
  {
    l4_timeout_s rcv;
    l4_timeout_s snd;
  } p;
} l4_timeout_t;


#define L4_IPC_TIMEOUT_0 ((l4_timeout_s){0x0400})
#define L4_IPC_TIMEOUT_NEVER ((l4_timeout_s){0})
#define L4_IPC_NEVER_INITIALIZER {0}
#define L4_IPC_NEVER ((l4_timeout_t){0})
#define L4_IPC_RECV_TIMEOUT_0 ((l4_timeout_t){0x00000400})
#define L4_IPC_SEND_TIMEOUT_0 ((l4_timeout_t){0x04000000})
#define L4_IPC_BOTH_TIMEOUT_0 ((l4_timeout_t){0x04000400})

/**
 * Timeout validities
 * \ingroup api_calls_fiasco
 *
 * times are actually 2^x values (e.g. 2ms -> 2048µs)
 */ 
enum l4_timeout_abs_validity {
  L4_TIMEOUT_ABS_V1_ms = 0,
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
  L4_TIMEOUT_ABS_V8_s,
  L4_TIMEOUT_ABS_V16_s,
  L4_TIMEOUT_ABS_V32_s,
};


L4_INLINE
l4_timeout_s l4_timeout_rel(unsigned man, unsigned exp);

L4_INLINE
l4_timeout_s l4_timeout_abs(l4_kernel_clock_t pint,
                    enum l4_timeout_abs_validity v);


/**
 * Convert explicit timeout values to L4 type
 * \ingroup api_calls_ipc
 *
 * \param  snd_man    Mantissa of send timeout.
 * \param  snd_exp    Exponent of send timeout.
 * \param  rcv_man    Mantissa of receive timeout.
 * \param  rcv_exp    Exponent of receive timeout.
 */

L4_INLINE
l4_timeout_t l4_ipc_timeout(unsigned snd_man, unsigned snd_exp,
    unsigned rcv_man, unsigned rcv_exp);

L4_INLINE
l4_timeout_t l4_timeout(l4_timeout_s snd, l4_timeout_s rcv);

L4_INLINE
void l4_snd_timeout(l4_timeout_s snd, l4_timeout_t *to);

L4_INLINE
void l4_rcv_timeout(l4_timeout_s rcv, l4_timeout_t *to);

L4_INLINE
l4_kernel_clock_t l4_timeout_rel_get(l4_timeout_s to);

L4_INLINE
l4_kernel_clock_t l4_timeout_abs_get(l4_kernel_clock_t cur, l4_timeout_s to);

L4_INLINE
unsigned l4_timeout_is_absolute(l4_timeout_s to);

L4_INLINE
l4_kernel_clock_t l4_timeout_get(l4_kernel_clock_t cur, l4_timeout_s to);


/*
 * Implementation
 */

L4_INLINE
l4_timeout_t l4_ipc_timeout(unsigned snd_man, unsigned snd_exp,
    unsigned rcv_man, unsigned rcv_exp)
{
  l4_timeout_t t;
  t.p.snd.t = (snd_man & 0x3ff) | ((snd_exp << 10) & 0x7c00);
  t.p.rcv.t = (rcv_man & 0x3ff) | ((rcv_exp << 10) & 0x7c00);
  return t;
}


L4_INLINE
l4_timeout_t l4_timeout(l4_timeout_s snd, l4_timeout_s rcv)
{
  l4_timeout_t t;
  t.p.snd = snd;
  t.p.rcv = rcv;
  return t;
}


L4_INLINE
void l4_snd_timeout(l4_timeout_s snd, l4_timeout_t *to)
{
  to->p.snd = snd;
}


L4_INLINE
void l4_rcv_timeout(l4_timeout_s rcv, l4_timeout_t *to)
{
  to->p.rcv = rcv;
}


L4_INLINE
l4_timeout_s l4_timeout_abs(l4_kernel_clock_t pint,
                    enum l4_timeout_abs_validity v)
{
  int e, m, c;
  l4_timeout_s to;

  e = v;
  m = pint >> e;
  c = (pint >> (e + 10)) & 1;
  m &= 0x3ff;
  
  to.t = 0x8000 | m | (e << 11) | (c << 10);
  return to;
}


L4_INLINE
l4_timeout_s l4_timeout_rel(unsigned man, unsigned exp)
{
  return (l4_timeout_s){(man & 0x3ff) | ((exp << 10) & 0x7c00)};
}


L4_INLINE
l4_kernel_clock_t l4_timeout_rel_get(l4_timeout_s to)
{
  if (to.t == 0)
    return ~0ULL;
  return (l4_kernel_clock_t)(to.t & 0x3ff) << ((to.t >> 10) & 0x1f);
}


L4_INLINE
l4_kernel_clock_t l4_timeout_abs_get(l4_kernel_clock_t cur, l4_timeout_s to)
{
  unsigned long e = (to.t >> 11) & 0xf;
  l4_kernel_clock_t timeout = (cur & ~((1 << (e + 10)) - 1)) | ((to.t & 0x3ff) << e);
  if (((cur >> (e + 10)) & 1) != ((to.t >> 10) & 1U))
    timeout += 1 << (e + 10);

  if (timeout < cur)
    return 0;

  return timeout;
}


L4_INLINE
unsigned l4_timeout_is_absolute(l4_timeout_s to)
{
  return to.t & 0x8000;
}


L4_INLINE
l4_kernel_clock_t l4_timeout_get(l4_kernel_clock_t cur, l4_timeout_s to)
{
  if (l4_timeout_is_absolute(to))
    return l4_timeout_abs_get(cur, to);
  else
    return cur + l4_timeout_rel_get(to);
}


#endif
