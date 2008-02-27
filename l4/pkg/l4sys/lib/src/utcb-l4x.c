/*!
 * \file   l4sys/lib/src/utcb.c
 * \brief  utcb for L4Linux programs
 *
 * \date   2008-02-25
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2008 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/sys/utcb.h>

l4_utcb_t *l4sys_utcb_get(void)
{
  l4_utcb_t *utcb;
  __asm__ __volatile__ ("mov %%fs:0, %0" : "=r" (utcb));
  return utcb;
}
