/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/include/__asm.h
 * \brief  Inline assembler stuff.
 *
 * \date   09/02/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _THREAD___ASM_H
#define _THREAD___ASM_H

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/cdefs.h>

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief Get current stack pointer.
 * 
 * \return stack pointer.
 */
/*****************************************************************************/ 
L4_INLINE l4_addr_t 
l4th_get_esp(void);

/*****************************************************************************
 *** implementation
 *****************************************************************************/

#ifdef ARCH_x86
/*****************************************************************************
 * l4th_get_esp, x86 version
 *****************************************************************************/
L4_INLINE l4_addr_t 
l4th_get_esp(void)
{
  l4_addr_t esp;

  asm("movl   %%esp, %0\n\t" : "=r" (esp) : );
  return esp;
}
#else

#error Unsupported hardware architecture!

#endif

#endif /* !_THREAD___ASM_H */
