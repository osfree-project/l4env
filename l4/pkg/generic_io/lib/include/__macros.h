/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_io/lib/include/__macros.h
 * \brief  L4Env I/O Client Library Support Macros
 *
 * \date   05/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __GENERIC_IO_LIB_INCLUDE___MACROS_H_
#define __GENERIC_IO_LIB_INCLUDE___MACROS_H_

/* L4 includes */
#include <l4/util/bitops.h>
#include <l4/util/macros.h>

#include "generic_io-client.h"

/* prototypes */
extern inline int nLOG2(l4_uint32_t);
extern inline int DICE_ERR(int, CORBA_Environment*);

/**
 * \brief LOG2(word) and round up
 */
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

/**
 * \brief test for dice or return error
 */
extern inline int DICE_ERR(int ret, CORBA_Environment *_env)
{
  if (ret || (_env->major != CORBA_NO_EXCEPTION))
    {
      ERROR("call failed (ret %d, exc %d) --- maybe this is okay for you!", ret, _env->major);
      return ret ? ret : -L4_EIPC;
    }
  else
    return 0;
}

#endif
