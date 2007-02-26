/**
 * \file   rt_mon/lib/client/comm.h
 * \brief  Interface for client applications to coordinator.
 *
 * \date   08/20/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __RT_MON_LIB_CLIENT_COMM_H_
#define __RT_MON_LIB_CLIENT_COMM_H_

int rt_mon_register_ds(l4dm_dataspace_t ds, const char * name);
int rt_mon_unregister_ds(int id);
int rt_mon_request_shared_ds(l4dm_dataspace_t * ds, int length,
                             const char * name, int * instance, void ** p);

#endif
