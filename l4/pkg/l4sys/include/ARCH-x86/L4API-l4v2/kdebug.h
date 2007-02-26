/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4sys/include/ARCH-x86/L4API-l4v2/kdebug.h
 * \brief   Kernel debugger macros
 * \ingroup api_calls
 */
/*****************************************************************************/
#ifndef __L4_L4v2_KDEBUG_H__
#define __L4_L4v2_KDEBUG_H__

#include <l4/sys/types.h>

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

/**
 * Register debugging symbols for a specific task. This allows the builtin
 * kernel debugger to show symbols when disassembling.
 * \ingroup api_calls_fiasco
 *
 * \param  tid		task ID of task the symbols belong to
 * \param  addr		start address of symbols. If 0, the symbols get
 * 			unregistered.
 */
L4_INLINE void
fiasco_register_symbols(l4_taskid_t tid, l4_addr_t addr, l4_size_t size);

/**
 * Register debugging lines for a specific task. This allows the builtin
 * kernel debugger to show lines when disassembling.
 * \ingroup api_calls_fiasco
 *
 * \param  tid		task ID of task the lines belong to
 * \param  addr		start address of lines. If 0, the lines get
 * 			unregistered.
 */
L4_INLINE void
fiasco_register_lines(l4_taskid_t tid, l4_addr_t addr, l4_size_t size);

/**
 * Register a thread names. This allows the builtin kernel debugger to
 * show thread names.
 * \ingroup api_calls_fiasco
 *
 * \param  tid		thread ID
 * \param  name		thread name. If this parameter is 0, the name
 * 			is unregistered.
 */
L4_INLINE void
fiasco_register_thread_name(l4_threadid_t tid, const char *name);

/**
 * Request CPU time consumed by a specific thread. 
 * \note The Fiasco config option "Fine-grained CPU time" should be
 *       activated to get precise values.
 * \ingroup api_calls_fiasco
 *
 * \param  tid		The ID of the thread whose CPU time should be
 * 			determined. To iterate through all existing threads
 * 			start with L4_NIL_ID == kernel thread.
 * \retval next_tid	Thread ID of next thread in present list. 
 * 			Pointer must not be NULL!
 * \retval total_us	Consumed CPU time in microseconds of thread specified
 * 			by tid. Pointer must not be NULL!
 * \retval prio		Priority of the thread specified by tid.
 * 			Pointer must not be NULL!
 * \return		0 on success, thread does not exists otherwise
 */
L4_INLINE int
fiasco_get_cputime(l4_threadid_t tid, l4_threadid_t *next_tid,
		   l4_uint64_t *total_us, l4_umword_t *prio);

/*****************************************************************************
 *** Implementation
 *****************************************************************************/

L4_INLINE void
fiasco_register_symbols(l4_taskid_t tid, l4_addr_t addr, l4_size_t size)
{
  asm("push %%ebx; movl %%esi,%%ebx \n\t"
      "int  $3; cmpb $30, %%al      \n\t"
      "pop  %%ebx                   \n\t"
      : : "a" (addr), "d" (size), "S" (tid.id.task), "c" (1));
}

L4_INLINE void
fiasco_register_lines(l4_taskid_t tid, l4_addr_t addr, l4_size_t size)
{
  asm("push %%ebx; movl %%esi,%%ebx \n\t"
      "int  $3; cmpb $30, %%al      \n\t"
      "pop  %%ebx                   \n\t"
      : : "a" (addr), "d" (size), "S" (tid.id.task), "c" (2));
}

L4_INLINE void
fiasco_register_thread_name(l4_threadid_t tid, const char *name)
{
  asm("int $3; cmpb $30, %%al" : : "a" (name), "c" (3), 
				   "S"(tid.lh.low), "D"(tid.lh.high));
}

L4_INLINE int
fiasco_get_cputime(l4_threadid_t tid, l4_threadid_t *next_tid,
		   l4_uint64_t *total, l4_umword_t *prio)
{
  l4_umword_t _prio;
  asm volatile ("int $3; cmpb $30, %%al"
		: "=A" (*total),
		  "=c" (_prio),
		  "=S" (next_tid->lh.low),
		  "=D" (next_tid->lh.high)
		: "c" (4),
		  "S" (tid.lh.low),
		  "D" (tid.lh.high));
  if (_prio == 0xffffffff)
    return _prio;

  *prio = _prio;
  return 0;
}

#endif

#include_next <l4/sys/kdebug.h>

