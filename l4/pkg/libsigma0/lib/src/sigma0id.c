/*!
 * \file   sigma0id.c
 * \brief  Get ID of Sigma0
 *
 * \date   2008-03-26
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2008 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/sigma0/sigma0.h>

l4_threadid_t l4sigma0_id(void)
{
  return (l4_threadid_t){.id = {.lthread = 0, .task = 2}};
}
