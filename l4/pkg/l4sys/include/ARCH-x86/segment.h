/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4sys/include/ARCH-x86/segment.h
 * \brief   Segment handling.
 * \ingroup api_calls
 */
/*****************************************************************************/
#ifndef __L4_SYS__ARCH_X86__SEGMENT_H__
#define __L4_SYS__ARCH_X86__SEGMENT_H__

#if !defined(L4API_l4v2) && !defined(L4API_l4x0)
#error This header file can only be used with a L4API version!
#endif

#include <l4/sys/types.h>

/**
 * Set LDT segments descriptors.
 * \ingroup api_calls_fiasco
 *
 * \param	ldt			Pointer to LDT hardware descriptors.
 * \param	size			Size of the descriptor in bytes
 *                                       (multiple of 8).
 * \param	entry_number_start	Entry number to start.
 * \param	task_nr			Task to set the segment for.
 */
L4_INLINE void
fiasco_ldt_set(void *ldt, unsigned int size,
               unsigned int entry_number_start, unsigned int task_nr);

/**
 * Set GDT segment descriptors. Fiasco supports 3 consecutive entries,
 * starting at FIASCO_GDT_ENTRY_OFFSET.
 * \ingroup api_calls_fiasco
 *
 * \param desc			Pointer to GDT descriptors.
 * \param size			Size of the descriptors in bytes
 *				 (multiple of 8).
 * \param entry_number_start	Entry number to start (valid values: 0-2).
 * \param tid			Thread ID to set the GDT entry for.
 */
L4_INLINE void
fiasco_gdt_set(void *desc, unsigned int size,
               unsigned int entry_number_start, l4_threadid_t tid);

/**
 * Return the offset of the entry in the GDT.
 * \ingroup api_calls_fiasco
 */
L4_INLINE unsigned
fiasco_gdt_get_entry_offset(void);

/*****************************************************************************
 *** Implementation
 *****************************************************************************/

L4_INLINE void
fiasco_ldt_set(void *ldt, unsigned int size,
               unsigned int entry_number_start, unsigned int task_nr)
{
  asm("lldt    %%ax    \n\t"
       : /* No output */
       : "a" (ldt),
         "b" (size),
	 "c" (entry_number_start),
	 "d" (task_nr)
      );
}

L4_INLINE unsigned
fiasco_gdt_get_entry_offset(void)
{
  unsigned offset;

  asm volatile("lldt	%%ax	\n\t"
               : "=b" (offset)
               : "b" (0));

  return offset;
}

#endif /* ! __L4_SYS__ARCH_X86__SEGMENT_H__ */
