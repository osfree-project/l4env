/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4io/server/include/__macros.h
 * \brief  L4Env l4io I/O Server Support Macros
 *
 * \date   05/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __L4IO_SERVER_INCLUDE___MACROS_H_
#define __L4IO_SERVER_INCLUDE___MACROS_H_

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
  tmp = l4util_bsr(word);
  /* round up */
  if (word > (1UL << tmp))
    tmp++;

  return tmp;
}

#endif
