/**
 * \file   rt_mon/server/coord/clients.h
 * \brief  Client handling.
 *
 * \date   08/20/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __RT_MON_SERVER_COORD_CLIENTS_H_
#define __RT_MON_SERVER_COORD_CLIENTS_H_

#include <l4/dm_generic/types.h>
#include <l4/rt_mon/types.h>

#define MAX_CLIENT_DSS 80

typedef struct
{
    int              usage_count;
    l4dm_dataspace_t ds;
    char             name[RT_MON_NAME_LENGTH];
    int              id;             // uniq. id to identify the sensor
    int              next_instance;  // next instance id to deliver
    int              shared;         // flag if sensor is shared
} coord_dss;

extern coord_dss client_dss[MAX_CLIENT_DSS];

void clients_init_data(void);
int clients_insert_data(const l4dm_dataspace_t *ds, const char* name);
int get_data_index(int id);
int get_data_for_name(const char * name);

#endif
