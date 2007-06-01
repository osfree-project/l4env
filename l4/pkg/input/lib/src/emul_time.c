/* $Id$ */
/*****************************************************************************/
/**
 * \file   input/lib/src/emul_time.c
 * \brief  L4INPUT: Linux time emulation
 *
 * \date   2007-05-29
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 */
#ifndef ARCH_arm
#include <l4/util/rdtsc.h>     /* XXX x86 specific */
#endif

/* Linux */
#include <asm/delay.h>

/* UDELAY */
void udelay(unsigned long usecs)
{
#ifdef ARCH_arm
  l4_sleep(usecs/1000); // XXX
#else
  l4_busy_wait_us(usecs);
#endif
}
