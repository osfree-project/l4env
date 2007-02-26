/* 
 * IA-64 L4 Types
 */


INTERFACE:


#include "types.h"
//#include <l4/sys/paging.h>

#ifndef L4_INLINE
#ifdef __cplusplus
#define L4_INLINE inline
#else
#define L4_INLINE extern __inline__
#endif
#endif

//typedef long long	    cpu_time_t;
//typedef __u64               mword_t;            


/*
typedef struct {
  dword_t low, high;
} l4_low_high_t;
*/


/* 
 * L4 unique identifiers 
 */

#define L4_NIL_ID 		((l4_threadid_t){val: 0})
#define L4_INVALID_ID		((l4_threadid_t){val: 0xffffffffffffffff})

/*
 * L4 flex pages
 */

#define L4_WHOLE_ADDRESS_SPACE (64)

/*
#define L4_PAGESIZE 	(0x1000)
#define L4_PAGEMASK	(~(L4_PAGESIZE - 1))
#define L4_LOG2_PAGESIZE (12)
#define L4_SUPERPAGESIZE (0x400000)
#define L4_SUPERPAGEMASK (~(L4_SUPERPAGESIZE - 1))
#define L4_LOG2_SUPERPAGESIZE (22)

#define L4_FPAGE_RO	0
#define L4_FPAGE_RW	1
#define L4_FPAGE_MAP	0
#define L4_FPAGE_GRANT	1
*/

/*
 * L4 message dopes
 */

/**
 * TODO unify IA32 and IA64 type !!!
 */

typedef struct {
  mowrd_t msg_deceited:1;
  mword_t fpage_received:1;
  mword_t msg_redirected:1;
  mword_t src_inside:1;
  mword_t error_code:4;
  mword_t strings:5;
  mword_t dwords:19;
} l4_msgdope_struct_t;



#define MSGDOPECC_FROM_EC(ec) ((l4_msgdope_t){md:{msg_deceited: 0, fpage_received: 0, \
  msg_redirected: 0, src_inside: 0, error_code: ec, strings: 0, dwords: 0}}).as_int



/*
 * L4 timeouts
 */

typedef struct {
  mword_t rcv_exp:8;
  mword_t snd_exp:8;
  mword_t rcv_pfault:8;
  mword_t snd_pfault:8;
  mword_t snd_man:16;
  mword_t rcv_man:16;
} l4_timeout_struct_t;



/* Compute l4_sched_param_struct_t->small argument for
   l4_thread_schedule(): size_mb is the size of all small address
   spaces, and nr is the number of the small address space.  See
   Liedtke: ``L4 Pentium implementation'' */
#define L4_SMALL_SPACE(size_mb, nr) ((size_mb >> 2) + nr * (size_mb >> 1))

/*
 * Some useful operations and test functions for id's and types
 */



// L4_INLINE int l4_is_invalid_sched_param(l4_sched_param_t sp);
// L4_INLINE int l4_is_nil_id(l4_threadid_t id);
// L4_INLINE int l4_is_invalid_id(l4_threadid_t id);
// L4_INLINE l4_fpage_t l4_fpage(unsigned long address, unsigned int size, 
// 			      unsigned char write, unsigned char grant);


// L4_INLINE l4_threadid_t get_taskid(l4_threadid_t t);
// L4_INLINE int thread_equal(l4_threadid_t t1,l4_threadid_t t2);
// L4_INLINE int task_equal(l4_threadid_t t1,l4_threadid_t t2);
// L4_INLINE void l4_make_taskid_from_irq(int irq, l4_threadid_t *t);
// L4_INLINE int l4_is_irq_id(l4_threadid_t t, unsigned irq_max) ;
// L4_INLINE int l4_irq_from_id(l4_threadid_t t);


// L4_INLINE int l4_is_invalid_sched_param(l4_sched_param_t sp)
// {
//   return sp.sched_param == 0xffffffffffffffff;
// }

// L4_INLINE int l4_is_nil_id(l4_threadid_t id)
// {
//   return id.val == 0;
// }

// L4_INLINE int l4_is_invalid_id(l4_threadid_t id)
// {
//   return id.val == 0xffffffffffffffff;
// }


// L4_INLINE l4_fpage_t l4_fpage(unsigned long address, unsigned int size, 
// 				  unsigned char write, unsigned char grant)
// {
//   return ((l4_fpage_t){fp:{grant, write, size, 0, 
// 			     (address & L4_PAGEMASK) >> 12 }});
// }


// L4_INLINE l4_threadid_t 
// get_taskid(l4_threadid_t t)
// {
//   t.id.lthread = 0;
//   return t; 
// }

// L4_INLINE int
// thread_equal(l4_threadid_t t1,l4_threadid_t t2)
// {
//   return t1.val == t2.val ? 1 : 0;
// }

// #define TASK_MASK 0xfffffffffffe03ff
// L4_INLINE int
// task_equal(l4_threadid_t t1,l4_threadid_t t2)
// {
//   if((t1.val & TASK_MASK) == (t2.val & TASK_MASK)) 
//     return 1;
//   else
//     return 0;
  
// /*   t1.id.lthread = 0; */
// /*   t2.id.lthread = 0; */

// /*   if((t1.lh.low != t2.lh.low) || (t1.lh.high != t2.lh.high)) */
// /*     return 0; */
// /*   return 1; */

// }

// L4_INLINE void
// l4_make_taskid_from_irq(int irq, l4_threadid_t *t)
// {
//   t->val = irq+1;
// }

// L4_INLINE int
// l4_is_irq_id(l4_threadid_t t, unsigned irq_max) 
// {
//   return (t.val <= irq_max);
// }

// L4_INLINE int
// l4_irq_from_id(l4_threadid_t t)
// {
//   return t.val -1;
// }


IMPLEMENTATION [arch]:

//-


