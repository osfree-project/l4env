/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/lib/src/global.c
 * \brief  Globally Required Stuff
 *
 * \date   08/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 */
#include <l4/lock/lock.h>

#include <l4/dde_linux/dde.h>

/* Linux */
#include <asm/system.h>

/* OSKit */
#include <stdlib.h>

/* local */
#include "__config.h"
#include "internal.h"

/** \name Synchronization
 *
 * Linux kernel synchronization is sometimes based on \c cli()/sti() pairs. As
 * discussed in "Taming Linux" simply disabling Interrupts via \c cli and \c
 * sti assembly statements is unacceptable in DROPS. The DDE uses the same lock
 * based scheme as a tamed L4Linux server.
 *
 * @{ */

static l4lock_t irq_lock = L4LOCK_UNLOCKED_INITIALIZER; //!< Interrupt lock

/** Global CLear Interrupt flag.
 *
 * Acquire #irq_lock. Do not grab it twice!
 */
void __global_cli()
{
  if (!l4thread_equal(l4thread_myself(), l4lock_owner(&irq_lock)))
    l4lock_lock(&irq_lock);
}

/** Global SeT Interrupt flag.
 *
 * Release #irq_lock.
 */
void __global_sti()
{
  l4lock_unlock(&irq_lock);
}

/** Global save flags.
 *
 * \return \a flags indicating action on __global_restore_flags(flag)
 *
 *  0 - global cli
 *  1 - global sti
 *  2 - local cli  <unused>
 *  3 - local sti  <unused>
 *
 * It is used to save the current #irq_lock state.
 * \sa __global_restore_flags
 */
unsigned long __global_save_flags()
{
  l4thread_t me = l4thread_myself();

  return l4thread_equal(me, l4lock_owner(&irq_lock)) ? 1 : 0;
}

/** Global restore flags.
 *
 * \param  flags  indicates action
 *
 * It restores #irq_lock state.
 * \sa __global_save_flags
 */
void __global_restore_flags(unsigned long flags)
{
  switch (flags)
    {
    case 0:
      __global_sti();
      break;

    case 1:
      __global_cli();
      break;
    default:
      DMSG("__global_restore_flags: unknown flags");
    }
}

/** @} */
/** \name Miscellaneous
 * @{ */

/** Parse integer from an option string
 * \ingroup mod_misc
 * (from lib/cmdline.c) */
int get_option (char **str, int *pint)
{
  char *cur = *str;

  if (!cur || !(*cur))
    return 0;
  *pint = strtol (cur, str, 0);
  if (cur == *str)
    return 0;
  if (**str == ',')
    {
      (*str)++;
      return 2;
    }

  return 1;
}

/**  Parse a string into a list of integers
 * \ingroup mod_misc
 * (from lib/cmdline.c) */
char *get_options (char *str, int nints, int *ints)
{
  int res, i = 1;

  while (i < nints)
  {
    res = get_option(&str, ints + i);
    if (res == 0)
      break;
    i++;
    if (res == 1)
      break;
  }
  ints[0] = i - 1;
  return str;
}

/** BIG kernel lock
 * \ingroup mod_misc */
#include <linux/spinlock.h>
spinlock_cacheline_t kernel_flag_cacheline = {SPIN_LOCK_UNLOCKED};

/** Delay some usecs
 * \ingroup mod_misc
 *
 * \todo consider l4_busy_wait_us() for short delays
 */
#include <asm/delay.h>
void udelay(unsigned long usecs)
{
  l4thread_usleep(usecs);
}
/** @} */
