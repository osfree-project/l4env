/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux/lib/include/__macros.h
 *
 * \brief	Library Support Macros
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

#ifndef _LIBDDE___MACROS_H
#define _LIBDDE___MACROS_H

#include <l4/sys/types.h>

#if 0
#include <l4/util/bitops.h>	/* would be the best */
#else

/** bit scan reverse */
static inline int
bsr(l4_uint32_t word)
{
  int tmp;

  if (word == 0)
    return -1;

  __asm__ __volatile__
    (
     "bsrl %1,%0 \n\t"
     :
     "=r" (tmp)       /* 0, index of most significant set bit */
     :
     "r"  (word)      /* 1, argument */
     );

  return tmp;
}

#endif

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

#endif /* !_LIBDDE___MACROS_H */
