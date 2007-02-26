/* $Id$ */
/*****************************************************************************/
/**
 * \file	l4io/server/include/__macros.h
 *
 * \brief	L4Env l4io I/O Server Support Macros
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

#ifndef _IO___MACROS_H
#define _IO___MACROS_H

/* L4 includes */
#include <l4/util/bitops.h>
#include <l4/util/macros.h>

/* prototypes */
extern inline int nLOG2(l4_uint32_t);

/** LOG2(word) and round up */
extern inline int nLOG2(l4_uint32_t word)
{
  int tmp;

  if (word == 0)
    return -1;

  /* log2 */
  tmp = bsr(word);
  /* round up */
  if (word > (1UL << tmp))
    tmp++;

  return tmp;
}

#endif /* !_IO___MACROS_H */
