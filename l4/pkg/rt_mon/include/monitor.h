/**
 * \file   rt_mon/include/monitor.h
 * \brief  Interface to coordinator from monitor.
 *
 * \date   08/20/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __RT_MON_INCLUDE_MONITOR_H_
#define __RT_MON_INCLUDE_MONITOR_H_

#include <l4/rt_mon/types.h>

int rt_mon_request_ds(int num, l4dm_dataspace_t * ds);
int rt_mon_release_ds(int id);
int rt_mon_list_ds(rt_mon_dss ** dss, int count);

#endif
