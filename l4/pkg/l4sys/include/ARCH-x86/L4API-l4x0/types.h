 /* 
 * $Id$
 */

#ifndef __L4_TYPES_H__ 
#define __L4_TYPES_H__ 

#include <l4/sys/l4int.h>
#include <l4/sys/compiler.h>
#include <l4/sys/consts.h>

typedef struct {
  l4_umword_t low, high;
} l4_low_high_t;

/* 
 * L4 unique identifiers 
 */

typedef struct {
  unsigned version_low:10;
  unsigned lthread:7;
  unsigned task:11;
  unsigned version_high:4;
  unsigned site:17;
  unsigned chief:11;
  unsigned nest:4;
} l4_threadid_struct_t;

typedef union {
  l4_low_high_t lh;
  l4_threadid_struct_t id;
} l4_threadid_t;

typedef l4_threadid_t l4_taskid_t;

typedef struct {
  unsigned intr:8;
  unsigned char zero[7];
} l4_intrid_struct_t;

typedef union {
  l4_low_high_t lh;
  l4_intrid_struct_t id;
} l4_intrid_t;

#define L4_NIL_ID 		((l4_threadid_t){lh:{0,0}})
#define L4_INVALID_ID		((l4_threadid_t){lh:{0xffffffff,0xffffffff}})

/*
 * L4 flex pages
 */

typedef struct {
  unsigned grant:1;
  unsigned write:1;
  unsigned size:6;
  unsigned zero:4;
  unsigned page:20;
} l4_fpage_struct_t;

typedef struct {
  unsigned grant:1;
  unsigned zero1:1;		/* no write permission on io ports */
  unsigned iosize:6;
  unsigned zero2:4;
  unsigned iopage:16;		/* base address */
  unsigned f: 4;		/* set to F for IO flex pages */
} l4_iofpage_struct_t;

typedef union {
  l4_umword_t fpage;
  l4_fpage_struct_t fp;
  l4_iofpage_struct_t iofp;
} l4_fpage_t;


#define L4_WHOLE_ADDRESS_SPACE	(32)
#define L4_FPAGE_RO		0
#define L4_FPAGE_RW		1
#define L4_FPAGE_MAP		0
#define L4_FPAGE_GRANT		1

#define L4_WHOLE_IOADDRESS_SPACE 16
#define L4_IOPORT_MAX  		(1L << L4_WHOLE_IOADDRESS_SPACE)

typedef struct {
  l4_umword_t snd_base;
  l4_fpage_t fpage;
} l4_snd_fpage_t;

/*
 * L4 message dopes
 */

typedef struct {
  unsigned msg_deceited:1;
  unsigned fpage_received:1;
  unsigned msg_redirected:1;
  unsigned src_inside:1;
  unsigned snd_error:1;
  unsigned error_code:3;
  unsigned strings:5;
  unsigned dwords:19;
} l4_msgdope_struct_t;

typedef union {
  l4_umword_t msgdope;
  l4_msgdope_struct_t md;
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

typedef struct {
  unsigned rcv_exp:4;
  unsigned snd_exp:4;
  unsigned rcv_pfault:4;
  unsigned snd_pfault:4;
  unsigned snd_man:8;
  unsigned rcv_man:8;
} l4_timeout_struct_t;

typedef union {
  l4_umword_t timeout;
  l4_timeout_struct_t to;
} l4_timeout_t;

/*
 * l4_schedule param word
 */

typedef struct {
  unsigned prio:8;
  unsigned small:8;
  unsigned state:4;
  unsigned time_exp:4;
  unsigned time_man:8;
} l4_sched_param_struct_t;

typedef union {
  l4_umword_t sched_param;
  l4_sched_param_struct_t sp;
} l4_sched_param_t;

#define L4_INVALID_SCHED_PARAM ((l4_sched_param_t){sched_param:(unsigned)-1})

/* Compute l4_sched_param_struct_t->small argument for
   l4_thread_schedule(): size_mb is the size of all small address
   spaces, and nr is the number of the small address space.  See
   Liedtke: ``L4 Pentium implementation'' */
#define L4_SMALL_SPACE(size_mb, nr) ((size_mb >> 2) + nr * (size_mb >> 1))

/*
 * Some useful operations and test functions for id's and types
 */

L4_INLINE int l4_is_invalid_sched_param(l4_sched_param_t sp);
L4_INLINE int l4_is_nil_id(l4_threadid_t id);
L4_INLINE int l4_is_invalid_id(l4_threadid_t id);
L4_INLINE l4_fpage_t l4_fpage(unsigned long address, unsigned int size, 
			      unsigned char write, unsigned char grant);
L4_INLINE l4_fpage_t l4_iofpage(unsigned port, unsigned int size, 
				unsigned char grant);
L4_INLINE int l4_is_io_page_fault(unsigned address);
L4_INLINE l4_threadid_t l4_get_taskid(l4_threadid_t t);
L4_INLINE int l4_thread_equal(l4_threadid_t t1,l4_threadid_t t2);
L4_INLINE int l4_task_equal(l4_threadid_t t1,l4_threadid_t t2);
L4_INLINE void l4_make_taskid_from_irq(int irq, l4_threadid_t *t);

L4_INLINE int l4_is_invalid_sched_param(l4_sched_param_t sp)
{
  return sp.sched_param == 0xffffffff;
}
L4_INLINE int l4_is_nil_id(l4_threadid_t id)
{
  return id.lh.low == 0;
}
L4_INLINE int l4_is_invalid_id(l4_threadid_t id)
{
  return id.lh.low == 0xffffffff;
}
L4_INLINE l4_fpage_t l4_fpage(unsigned long address, unsigned int size, 
			      unsigned char write, unsigned char grant)
{
  return ((l4_fpage_t){fp:{grant, write, size, 0, 
			     (address & L4_PAGEMASK) >> 12 }});
}

L4_INLINE l4_fpage_t l4_iofpage(unsigned port, unsigned int size, 
				unsigned char grant)
{
  return ((l4_fpage_t){iofp:{grant, 0, size, 0, port, 0xf}});
}

L4_INLINE int l4_is_io_page_fault(unsigned address)
{
  l4_fpage_t t;
  t.fpage = address;
  return(t.iofp.f == 0xf);
}

L4_INLINE l4_threadid_t 
l4_get_taskid(l4_threadid_t t)
{
  t.id.lthread = 0;
  return t; 
}

L4_INLINE int
l4_thread_equal(l4_threadid_t t1,l4_threadid_t t2)
{
  if((t1.lh.low != t2.lh.low) || (t1.lh.high != t2.lh.high))
    return 0;
  return 1;
}

#define TASK_MASK 0xfffe03ff
L4_INLINE int
l4_task_equal(l4_threadid_t t1,l4_threadid_t t2)
{
  if ( ((t1.lh.low & TASK_MASK) == (t2.lh.low & TASK_MASK)) && 
       (t1.lh.high == t2.lh.high) )
    return 1;
  else
    return 0;
  
/*   t1.id.lthread = 0; */
/*   t2.id.lthread = 0; */

/*   if((t1.lh.low != t2.lh.low) || (t1.lh.high != t2.lh.high)) */
/*     return 0; */
/*   return 1; */

}

L4_INLINE void
l4_make_taskid_from_irq(int irq, l4_threadid_t *t)
{
  t->lh.low = irq+1;
  t->lh.high = 0;
}


#endif /* __L4_TYPES_H__ */ 


