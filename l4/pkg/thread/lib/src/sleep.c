/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/src/sleep.c
 * \brief  Thread sleep.
 *
 * \date   12/28/2000
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
 *
 * \todo Synchronize with L4 kernel timer (requires abstraction of the 
 *       kernel clock in l4env).
 */
/*****************************************************************************/

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/util/bitops.h>
#include <l4/util/macros.h>

/* library includes */
#include <l4/thread/thread.h>
#include "__asm.h"
#include "__debug.h"

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Calculate L4 timeout.
 * 
 * \param  mus           Timeout (microseconds)
 * \retval to_e          Exponent of the L4 timeout 
 * \retval to_m          Mantissa of the L4 timeout
 *
 * Calculate the exponent/mantissa of the L4 send/receive timeout from the 
 * microsecond value. 
 * 
 * The L4 timeout is specified by 2 unsigned integer values e (4 bit) and
 * m (8 bit) (see L4 Reference Manual) where 
 * 
 * \f[ timeout (\mu s)  = m * 4^{(15 - e)}\f]
 * 
 * With a given timeout \a mus, \a m and \a e can be calculated as follows:
 *
 * \f[ e = 14 - \left\lfloor \frac{1}{2} 
 *         log_2\left(\frac{mus}{256}\right) \right\rfloor \f]
 * \f[ m = \frac{mus}{2^{2 * (15 - e)}} \f]
 */
/*****************************************************************************/ 
static inline void 
__micros2l4to(l4_uint32_t mus, l4_uint32_t * to_e, l4_uint32_t * to_m)
{
  if (mus == 0)
    {
      /* timeout 0, m = 0, e != 0 (see L4 Reference Manual) */  
      *to_e = 1;
      *to_m = 0; 
    } 
  else 
    { 
      *to_e = 14 - bsr(mus / 256) / 2;
      *to_m = mus / (1UL << (2 * (15 - *to_e)));

      /* sanity check */
      if ((*to_e > 15) || (*to_m > 255))
	{
	  Error("l4thread: invalid timeout (%u), using max. values",mus);
	  *to_e = 0;
	  *to_m = 255;
	}
    }

#if DEBUG_SLEEP
  INFO("mus = %u -> e = %u, m = %u\n",mus,*to_e,*to_m);
#endif 
}

/*****************************************************************************/
/**
 * \brief  Sleep.
 * 
 * \param  t             Timeout (microseconds)
 *
 * Do sleep.
 *
 * \todo Restart sleep if IPC canceled.
 */
/*****************************************************************************/ 
static void
__do_sleep(l4_uint32_t t)
{
  l4_uint32_t to_e,to_m;
  l4_timeout_t to;
  int error;
  l4_umword_t dummy;
  l4_msgdope_t result;

  /* calculate timeout */
  __micros2l4to(t,&to_e,&to_m);

  /* sanity check */
  if (to_e && !to_m)
    /* sleep(0us), nothing to do */
    return;

  to = L4_IPC_TIMEOUT(0,0,to_m,to_e,0,0);

  /* do wait */
  error = l4_i386_ipc_receive(L4_NIL_ID,L4_IPC_SHORT_MSG,
                              &dummy,&dummy,to,&result);

  if (error != L4_IPC_RETIMEOUT)
    Error("l4thread: sleep canceled!");
}

/*****************************************************************************
 *** l4thread user API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Sleep.
 * 
 * \param  t             Time (milliseconds)
 *
 * Sleep for \t milliseconds.
 */
/*****************************************************************************/ 
void
l4thread_sleep(l4_uint32_t t)
{
  /* sleep */
  __do_sleep(t * 1000);
}

/*****************************************************************************/
/**
 * \brief  Sleep.
 * 
 * \param  t             Time (microseconds).
 *
 * Sleep for \t microseconds.
 *
 * \note Although the L4 timeout is specified in microseconds, the actual 
 *       timer resolution is about one millisecond. If we really need 
 *       microsecond timers, we must implement them differently.
 * \todo Implement microsecond timers (if we really need them).
 */
/*****************************************************************************/ 
void 
l4thread_usleep(l4_uint32_t t)
{
  /* sleep */
  __do_sleep(t);
}

