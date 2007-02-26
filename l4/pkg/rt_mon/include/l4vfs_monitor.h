/**
 * \file   rt_mon/include/l4vfs_monitor.h
 * \brief  Interface to l4vfs_coord from monitor.
 *
 * \date   10/26/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __RT_MON_INCLUDE_L4VFS_MONITOR_H_
#define __RT_MON_INCLUDE_L4VFS_MONITOR_H_

#include <l4/rt_mon/types.h>

/** 
 * @brief Request and open a sensor at coordinator, attach its memory
 * 
 * @retval p    pointer to attached memory reagion
 * @param  name filename of the sensor to request
 * 
 * @return - file descriptor number on success
 *         - -1 on error, errno will be set to error code
 */
int rt_mon_request_ds(void ** p, const char * name);

/** 
 * @brief Release and close sensors, detache its memory
 * 
 * @param fd file descriptor to release
 * 
 * @return - 0 on success
 *         - -1 otherwise, errno will be set to errorcode
 */
int rt_mon_release_ds(int fd);

#endif
