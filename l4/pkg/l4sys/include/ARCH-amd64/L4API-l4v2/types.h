/*****************************************************************************/
/**
 * \file    l4sys/include/ARCH-x86/L4API-l4v2/types.h
 * \brief   L4 kernel API type definitions, L4 v2 version
 * \ingroup api_types
 */
/*****************************************************************************/
#ifndef __L4_TYPES_H__
#define __L4_TYPES_H__

#include <l4/sys/compiler.h>
#include <l4/sys/consts.h>
#include <l4/sys/l4int.h>
#include_next <l4/sys/types.h>

/**
 * 64 Bit -> 32 Bit low/high conversion
 * \ingroup api_types_common
 */
typedef struct {
  l4_uint32_t low;  ///< Low 32 Bits
  l4_uint32_t high; ///< High 32 Bits
} l4_low_high_t;

/*****************************************************************************
 *** L4 unique identifiers
 *****************************************************************************/

/**
 * L4 thread id structure
 * \ingroup api_types_id
 */
typedef struct {
  unsigned version_low:10;   ///< Version0
  unsigned lthread:7;        ///< Thread number
  unsigned task:11;          ///< Task number
  unsigned version_high:4;   ///< Version1
} l4_threadid_struct_t;

/**
 * L4 thread id
 * \ingroup api_types_id
 */
typedef union {
  l4_uint32_t raw;           ///< raw id
  l4_threadid_struct_t id;   ///< Thread id struct
} l4_threadid_t;

/**
 * L4 task id
 * \ingroup api_types_id
 */
typedef l4_threadid_t l4_taskid_t;

/**
 * L4 interrupt id structure
 * \ingroup api_types_id
 */
typedef struct {
  unsigned intr:8;           ///< Interrupt number
  unsigned char zero[3];     ///< Unused (must be 0)
} l4_intrid_struct_t;

/**
 * L4 interrupt id
 * \ingroup api_types_id
 */
typedef union {
  l4_intrid_struct_t id;     ///< Interrupt id struct
} l4_intrid_t;

/**
 * L4 nil thread id
 * \ingroup api_types_id
 * \hideinitializer
 */
#define L4_NIL_ID_INIT	     {0}
#define L4_NIL_ID	     ((l4_threadid_t)L4_NIL_ID_INIT)

/**
 * L4 invalid thread id
 * \ingroup api_types_id
 * \hideinitializer
 */
#define L4_INVALID_ID_INIT   {0xffffffff}
#define L4_INVALID_ID        ((l4_threadid_t)L4_INVALID_ID_INIT)

/**
 * \brief   Test if \a id is nil thread id
 * \ingroup api_types_id
 *
 * \param   id           Thread id
 * \return  != 0 if \a id is nil thread id, 0 if not
 */
L4_INLINE int
l4_is_nil_id(l4_threadid_t id);

/**
 * \brief   Test if \a id is invalid thread id
 * \ingroup api_types_id
 *
 * \param   id           Thread id
 * \return  != 0 if \a id is invalid thread id, 0 if not
 */
L4_INLINE int
l4_is_invalid_id(l4_threadid_t id);

/**
 * \brief   Test if \a id is a IRQ thread id
 * \ingroup api_types_id
 *
 * \param   id           Thread id
 * \return  != 0 if \a id is IRQ thread id, 0 if not
 */
L4_INLINE int
l4_is_irq_id(l4_threadid_t id);

/**
 * \brief   Return the IRQ number
 * \ingroup api_types_id
 *
 * \param   id            Thread id
 * \return  IRQ number
 */
L4_INLINE int
l4_get_irqnr(l4_threadid_t id);

/**
 * \brief   Create task id from thread id
 * \ingroup api_types_id
 *
 * \param   t            Thread id
 * \return  Task id
 */
L4_INLINE l4_threadid_t
l4_get_taskid(l4_threadid_t t);

/**
 * \brief   Test if two thread ids are equal
 * \ingroup api_types_id
 *
 * \param   t1           First thread id
 * \param   t2           Second thread id
 * \return  !=0 if thread ids are equal, 0 if not
 */
L4_INLINE int
l4_thread_equal(l4_threadid_t t1, l4_threadid_t t2);

/**
 * \brief   Test if two task ids are equal
 * \ingroup api_types_id
 *
 * \param   t1           First task id
 * \param   t2           Second task id
 * \return  != 0 if task ids are equal, 0 if not
 */
L4_INLINE int
l4_task_equal(l4_threadid_t t1, l4_threadid_t t2);

/**
 * \brief   Test if the task numbers of two task ids are equal
 * \ingroup api_types_id
 *
 * \param   t1           First task id
 * \param   t2           Second task id
 * \return  != 0 if task numbers are equal, 0 if not
 */
L4_INLINE int
l4_tasknum_equal(l4_threadid_t t1, l4_threadid_t t2);

/**
 * \brief   Create interrupt id
 * \ingroup api_types_id
 *
 * \param   irq          Interrupt number
 * \retval  t            Interrupt id
 */
L4_INLINE void
l4_make_taskid_from_irq(int irq, l4_threadid_t *t);

/*****************************************************************************
 *** L4 flexpages
 *****************************************************************************/

#include <l4/sys/__l4_fpage-64bit.h>

/*****************************************************************************
 *** L4 message dopes
 *****************************************************************************/

/**
 * L4 message dope structure
 * \ingroup api_types_msg
 */
typedef struct {
  unsigned msg_deceited:1;   ///< Received message deceited (unused)
  unsigned fpage_received:1; ///< Received flexpage
  unsigned msg_redirected:1; ///< Received message redirected (unused)
  unsigned src_inside:1;     ///< Received message from an inner clan (unused)
  unsigned snd_error:1;      ///< Error during send operation
  unsigned error_code:3;     ///< Error code
  unsigned strings:5;        ///< Number of indirect strings in message
  unsigned dwords:19;        ///< Number of dwords in message
} l4_msgdope_struct_t;


/**
 * L4 message dope type
 * \ingroup api_types_msg
 */
typedef union {
  l4_umword_t msgdope;       ///< Plain 32 Bit value
  l4_msgdope_struct_t md;    ///< Message dope structure
  l4_umword_t raw;
} l4_msgdope_t;

/**
 * L4 string dope
 * \ingroup api_types_msg
 */
typedef struct {
  l4_umword_t snd_size;      ///< Send string size
  l4_umword_t snd_str;       ///< Send string address
  l4_umword_t rcv_size;      ///< Receive string size
  l4_umword_t rcv_str;       ///< Receive string address
} l4_strdope_t;

/*****************************************************************************
 *** L4 timeouts
 *****************************************************************************/

#include <l4/sys/__timeout.h>

/*****************************************************************************
 *** l4_schedule param word
 *****************************************************************************/

/**
 * Scheduling parameter structure
 * \ingroup api_types_sched
 */
typedef struct {
  unsigned prio:8;           /**< System-wide priority of the destination
                              **  thread. 255 is the highest and 0 the lowest
                              **  priority.
                              **/
  unsigned small:8;          /**< Small address space number for destination
                              **  task (see #L4_SMALL_SPACE)
                              **/
  unsigned state:4;          /**< Thread state (l4_thread_schedule() return
                              **  value)
                              **
                              **  Values:
                              **  - 0+\a k \a Running. The thread is ready to
                              **    execute at user-level.
                              **  - 4+\a k \a Sending. A user-invoked IPC send
                              **    operation currently transfers an outgoing
                              **    message.
                              **  - 8+\a k \a Receiving. A user-invoked IPC
                              **    receive operation currently receives an
                              **    incoming message.
                              **  - 0xC \a Waiting for receive. A user-invoked
                              **    receive operation currently waits for an
                              **    incoming message.
                              **  - 0xD \a Pending send. A user-invoked send
                              **    operation currently waits for the
                              **    destination (recipient) to become ready to
                              **    receive.
                              **  - 0xE \a Reserved.
                              **  - 0xF \a Dead. The thread is unable to
                              **    execute.
                              **
                              **  - k=0 \a Kernel \a inactive. The kernel does
                              **    not execute an automatic RPC for the thread.
                              **  - k=1 \a Pager. The kernel executes a
                              **    pagefault RPC to the thread's pager.
                              **  - k=2 \a Internal \a preempter. The kernel
                              **    executes a preemption RPC to the thread's
                              **    internal preempter.
                              **  - k=3 \a external \a preempter. The kernel
                              **    executes a preemption RPC to the thread's
                              **    external preempter.
                              **
                              **  If l4_sched_param_struct_t is used as an input
                              **  argument for l4_thread_schedule(), \a state
                              *   must be 0.
                              **/
  unsigned time_exp:4;       ///< Timeslice exponent
  unsigned time_man:8;       ///< Timeslice mantissa
} l4_sched_param_struct_t;

/**
 * Scheduling parameter type
 * \ingroup api_types_sched
 */
typedef union {
  l4_umword_t sched_param;   ///< Plain 32 bit value
  l4_sched_param_struct_t sp;///< Scheduling parameter structure
} l4_sched_param_t;


/**
 * Quota type structure
 */
typedef union l4_quota_desc_t
{
  l4_umword_t raw;
  struct
  {
    unsigned long id: 12;
    unsigned long amount: 16;
    unsigned long cmd: 4;
  } q;
} l4_quota_desc_t;


/**
 * Invalid scheduling parameter
 * \ingroup api_types_sched
 * \hideinitializer
 */
#define L4_INVALID_SCHED_PARAM ((l4_sched_param_t){sched_param:~0UL})

/**
 * Compute l4_sched_param_struct_t->small argument
 * \ingroup api_types_sched
 * \hideinitializer
 *
 * \param   size_mb      Small space size (MB), possible sizes are
 *                       2, 4, 8, ... 256 megabytes
 * \param   nr           Small space number, valid numbers are
 *                       1 .. (512/\a size_mb)-1.
 *
 * Compute l4_sched_param_struct_t->small argument for
 * l4_thread_schedule(): size_mb is the size of all small address
 * spaces, and nr is the number of the small address space.  See
 * Liedtke: ``L4 Pentium implementation''
 */
#define L4_SMALL_SPACE(size_mb, nr) ((size_mb >> 2) + nr * (size_mb >> 1))

/**
 * \brief   Test if \a sp is invalid scheduling parameter
 * \ingroup api_types_sched
 *
 * \param   sp           Scheduling parameter
 * \return  != 0 if \a sp is invalid scheduling parameter, 0 if not
 */
L4_INLINE int
l4_is_invalid_sched_param(l4_sched_param_t sp);

L4_INLINE void
l4_sched_param_set_time(int us, l4_sched_param_t *p);



/*****************************************************************************
 *** implementations
 *****************************************************************************/

L4_INLINE int
l4_is_invalid_sched_param(l4_sched_param_t sp)
{
  return sp.sched_param == 0xffffffff;
}

L4_INLINE int
l4_is_nil_id(l4_threadid_t id)
{
  return id.raw == 0;
}

L4_INLINE int
l4_is_invalid_id(l4_threadid_t id)
{
  return id.raw == 0xffffffff;
}

L4_INLINE int
l4_is_irq_id(l4_threadid_t id)
{
  return id.raw > 0 && id.raw <= 255;
}

L4_INLINE int
l4_get_irqnr(l4_threadid_t id)
{
  return id.raw - 1;
}

L4_INLINE l4_threadid_t
l4_get_taskid(l4_threadid_t t)
{
  t.id.lthread = 0;
  return t;
}

L4_INLINE int
l4_thread_equal(l4_threadid_t t1, l4_threadid_t t2)
{
  return t1.raw == t2.raw;
}

#define TASK_MASK 0xfffe03ff
L4_INLINE int
l4_task_equal(l4_threadid_t t1, l4_threadid_t t2)
{
  return (t1.raw & TASK_MASK) == (t2.raw & TASK_MASK);
}

L4_INLINE int
l4_tasknum_equal(l4_threadid_t t1, l4_threadid_t t2)
{
  return (t1.id.task == t2.id.task);
}

L4_INLINE void
l4_make_taskid_from_irq(int irq, l4_threadid_t *t)
{
  t->raw = irq+1;
}

L4_INLINE void
l4_sched_param_set_time(int us, l4_sched_param_t *p)
{
  unsigned m = us;
  unsigned e = 15;

  while (m > 255)
    {
      e--;
      m >>= 2;
    }
  p->sp.time_man = m;
  p->sp.time_exp = e;
}

#endif /* !__L4_TYPES_H__ */
