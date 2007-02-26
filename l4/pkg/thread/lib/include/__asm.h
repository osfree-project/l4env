/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/include/__asm.h
 * \brief  Inline assembler stuff.
 *
 * \date   09/02/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/
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
