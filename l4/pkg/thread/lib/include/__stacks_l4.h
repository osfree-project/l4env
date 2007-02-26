/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/include/__stacks_l4.h
 * \brief  Stack handling, architecture depended implementation (L4 version)
 *
 * \date   04/10/2002
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
#ifndef _THREAD___STACKS_L4_H
#define _THREAD___STACKS_L4_H

/* library includes */
#include "__asm.h"

/*****************************************************************************/
/**
 * \brief  Return thread id of current thread
 *	
 * \return thread id, -1 if thread not found (=> stack outside stack area)
 */
/*****************************************************************************/ 
L4_INLINE int
l4th_stack_get_current_id(void)
{
  l4_addr_t esp;

  if (!l4th_have_stack_area)
    /* we do not use a special stack area, thus can not calculate thread 
     * number from stack pointer */
    return -1;

  esp = l4th_get_esp();
  if ((esp >= l4th_stack_area_start) && (esp <= l4th_stack_area_end))
    /* can calculate thread no from stack pointer */
    return (esp - l4th_stack_area_start) / l4thread_max_stack;
  else
    /* user defined stack */
    return -1;
}

#endif /* !_THREAD___STACKS_L4_H */
