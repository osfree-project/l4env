/*!
 * \file   l4sys/lib/src/utcb.c
 * \brief  utcb
 *
 * \date   2007-11-20
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2007 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/sys/utcb.h>

__attribute__((weak)) l4_utcb_t *l4sys_utcb_get(void)
{
  return l4_utcb_get();
}
