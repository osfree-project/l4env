/* $Id$ */
/*****************************************************************************/
/**
 * \file	generic_io/lib/include/__macros.h
 *
 * \brief	L4Env I/O Client Library Support Macros
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

#ifndef _LIBIO___MACROS_H
#define _LIBIO___MACROS_H

/* L4 includes */
#include <l4/util/bitops.h>
#include <l4/util/macros.h>

#include "generic_io-client.h"

/* prototypes */
extern inline int nLOG2(l4_uint32_t);
extern inline int FLICK_ERR(int, sm_exc_t*);

/**
 * \brief LOG2(word) and round up
 */
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

/**
 * \brief test for flick or return error
 */
extern inline int FLICK_ERR(int ret, sm_exc_t *exc)
{
  if (ret || (exc->_type != exc_l4_no_exception))
    {
      ERROR("call failed (ret %d, exc %d) --- maybe this is okay for you!", ret, exc->_type);
      return ret ? ret : -L4_EIPC;
    }
  else
    return 0;
}

#endif /* !_LIBIO___MACROS_H */
