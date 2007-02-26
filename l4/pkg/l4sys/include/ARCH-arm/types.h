 /* 
 * $Id$
 */

#ifndef L4_TYPES_H 
#define L4_TYPES_H

#include <l4/sys/l4int.h>
#include <l4/sys/compiler.h>
#include <l4/sys/consts.h>

#if defined(L4API_l4x0)
#include <l4/sys/types_api.h>
#endif

/*
 * L4 flex pages
 */
typedef union {
  struct {
    l4_umword_t grant : 1;
    l4_umword_t write : 1;
    l4_umword_t size  : 6;
    l4_umword_t zero  : 4;
    l4_umword_t page  :20;
  } fp;
  l4_umword_t raw;
  l4_umword_t fpage;
} l4_fpage_t;


#define L4_WHOLE_ADDRESS_SPACE	(32)
#define L4_FPAGE_RO		0
#define L4_FPAGE_RW		1
#define L4_FPAGE_MAP		0
#define L4_FPAGE_GRANT		1

typedef struct {
  l4_umword_t snd_base;
  l4_fpage_t fpage;
} l4_snd_fpage_t;

/*
 * L4 message dopes
 */
typedef union {
  struct {
    l4_umword_t msg_deceited   : 1;
    l4_umword_t fpage_received : 1;
    l4_umword_t msg_redirected : 1;
    l4_umword_t src_inside     : 1;
    l4_umword_t snd_error      : 1;
    l4_umword_t error_code     : 3;
    l4_umword_t strings        : 5;
    l4_umword_t dwords         :19;
  } md;
  l4_umword_t msgdope;
  l4_umword_t raw;
} l4_msgdope_t;

/*
 * L4 string dopes
 */
typedef struct {
  l4_umword_t snd_size;
  l4_umword_t snd_str;
  l4_umword_t rcv_size;
  l4_umword_t rcv_str;
} l4_strdope_t;

/*
 * L4 timeouts
 */
typedef union {
  struct {
    l4_umword_t rcv_exp    :4;
    l4_umword_t snd_exp    :4;
    l4_umword_t rcv_pfault :4;
    l4_umword_t snd_pfault :4;
    l4_umword_t snd_man    :8;
    l4_umword_t rcv_man    :8;
  } to;
  l4_umword_t timeout;
  l4_umword_t raw;
} l4_timeout_t;

/*
 * l4_schedule param word
 */
typedef union {
  struct {
    l4_umword_t prio     :8;
    l4_umword_t small    :8;
    l4_umword_t state    :4;
    l4_umword_t time_exp :4;
    l4_umword_t time_man :8;
  } sp;
  l4_umword_t sched_param;
  l4_umword_t raw;
} l4_sched_param_t;

#define L4_INVALID_SCHED_PARAM ((l4_sched_param_t){raw:(l4_umword_t)-1})

/* Compute l4_sched_param_struct_t->small argument for
   l4_thread_schedule(): size_mb is the size of all small address
   spaces, and nr is the number of the small address space.  See
   Liedtke: ``L4 Pentium implementation'' */
#define L4_SMALL_SPACE(size_mb, nr) ((size_mb >> 2) + nr * (size_mb >> 1))


typedef struct {
  l4_fpage_t   fpage;
  l4_msgdope_t size;
  l4_msgdope_t send;
  l4_umword_t  word[1];
} l4_msg_t;


typedef struct {
  l4_umword_t low, high;
} l4_low_high_t;

/*
 * Some useful operations and test functions for id's and types
 */

L4_INLINE int        l4_is_invalid_sched_param (l4_sched_param_t sp);
L4_INLINE l4_fpage_t l4_fpage(l4_addr_t address, l4_size_t size, 
                              unsigned char write, unsigned char grant);

/*-----------------------------------------------------------------------------
 * IMPLEMENTATION
 *---------------------------------------------------------------------------*/
L4_INLINE int l4_is_invalid_sched_param(l4_sched_param_t sp)
{
  return sp.raw == (l4_umword_t)-1;
}

L4_INLINE l4_fpage_t l4_fpage(l4_addr_t address, l4_size_t size, 
			      unsigned char write, unsigned char grant)
{
  return ((l4_fpage_t){fp:{grant, write, size, 0, 
			     (address & L4_PAGEMASK) >> 12 }});
}

#endif /* L4_TYPES_H */ 


