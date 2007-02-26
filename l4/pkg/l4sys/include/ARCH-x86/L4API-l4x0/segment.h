#include_next <l4/sys/segment.h>

/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4sys/include/ARCH-x86/L4API-l4x0/segment.h
 * \brief   l4x0 specific segment manipulation
 * \ingroup api_calls_fiasco
 */
/*****************************************************************************/
#ifndef __L4_SYS__ARCH_X86__L4API_L4X0__SEGMENT_H__
#define __L4_SYS__ARCH_X86__L4API_L4X0__SEGMENT_H__

#include <l4/sys/compiler.h>
#include <l4/sys/xadaption.h>

/*****************************************************************************
 *** Implementation
 *****************************************************************************/

L4_INLINE void
fiasco_gdt_set(void *desc, unsigned int size,
	       unsigned int entry_number_start, l4_threadid_t tid)
{
  asm(ToId32_EdiEsi
      "lldt %%ax"
       :  /* No output */
       : "a"(desc),
	 "b"(size),
         "c"(entry_number_start),
	 "d"(0),
	 "S"(tid.lh.low),
	 "D"(tid.lh.high));
}

#endif /* ! __L4_SYS__ARCH_X86__L4API_L4X0__SEGMENT_H__ */
