/* $Id$ */
/**
 * \file    rtc/server/src/ux.c
 * \brief   Get current time
 *
 * \date    09/26/2003
 * \author  Adam Lackorzynski <adam@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/lxfuxlibc/lxfuxlc.h>

#include <l4/util/rdtsc.h>
#include <l4/util/irq.h>

extern l4_uint32_t system_time_offs_rel_1970;
void get_base_time(void);

void
get_base_time(void)
{
  l4_uint32_t current_s, current_ns;

  l4_tsc_to_s_and_ns(l4_rdtsc(), &current_s, &current_ns);
  system_time_offs_rel_1970 = lx_time(NULL) - current_s;
}

