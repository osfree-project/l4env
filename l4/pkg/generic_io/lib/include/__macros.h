/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_io/lib/include/__macros.h
 * \brief  L4Env I/O Client Library Support Macros
 *
 * \date   2007-03-23
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

/**
 * @brief The minimal page size used for client IO memory requests.
 *
 * This size used to be super page size and is now page size by default.
 */
#define GENERIC_IO_MIN_PAGEORDER L4_LOG2_PAGESIZE

/* prototypes */
extern inline int nLOG2(unsigned long);
extern inline int DICE_ERR(int, CORBA_Environment*);
extern inline unsigned long generic_io_trunc_page(unsigned long);

/**
 * \brief LOG2(word) and round up
 */
extern inline int nLOG2(unsigned long word)
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
  if (DICE_HAS_EXCEPTION(_env)
#ifdef DEBUG_ERRORS
    || ret
#endif
  )
    {
      LOG_Error("call failed (ret %d \"%s\", exc %d) --- maybe this is okay for you!",
                ret, l4env_strerror(-ret), DICE_EXCEPTION_MAJOR(_env));
      return ret ? ret : -L4_EIPC;
    }
  else
    return ret;
}


extern inline
unsigned long generic_io_trunc_page(unsigned long addr)
{
  return addr & ~((1UL << GENERIC_IO_MIN_PAGEORDER) - 1);
}

#endif
