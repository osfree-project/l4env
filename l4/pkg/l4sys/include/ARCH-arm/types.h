 /* 
 * $Id$
 */

#ifndef L4_TYPES_H 
#define L4_TYPES_H

#include_next <l4/sys/types.h>

#include <l4/sys/l4int.h>
#include <l4/sys/compiler.h>
#include <l4/sys/consts.h>

#if defined(L4API_l4v2)
#include <l4/sys/types_api.h>
#endif

#include <l4/sys/__l4_fpage-32bit.h>

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

#include <l4/sys/__timeout.h>

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
  l4_umword_t low, high;
} l4_low_high_t;

/*
 * Some useful operations and test functions for id's and types
 */

L4_INLINE int        l4_is_invalid_sched_param (l4_sched_param_t sp);

/*-----------------------------------------------------------------------------
 * IMPLEMENTATION
 *---------------------------------------------------------------------------*/
L4_INLINE int l4_is_invalid_sched_param(l4_sched_param_t sp)
{
  return sp.raw == (l4_umword_t)-1;
}


#endif /* L4_TYPES_H */ 


