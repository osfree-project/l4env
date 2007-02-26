/**
 * \file   rt_mon/server/coord/clients.c
 * \brief  Client handling.
 *
 * \date   08/20/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <string.h>

#include "clients.h"

coord_dss client_dss[MAX_CLIENT_DSS];

static int id_counter = 0;

void clients_init_data(void)
{
    int i;
    for (i = 0; i < MAX_CLIENT_DSS; i++)
    {
        client_dss[i].usage_count = 0;
    }
}

int clients_insert_data(const l4dm_dataspace_t *ds, const char* name)
{
    int i;
    for (i = 0; i < MAX_CLIENT_DSS; i++)
    {
        if (client_dss[i].usage_count <= 0)
        {
            client_dss[i].ds = *ds;
            strncpy(client_dss[i].name, name, RT_MON_NAME_LENGTH - 1);
            client_dss[i].name[RT_MON_NAME_LENGTH - 1] = 0;
            client_dss[i].usage_count   = 1;
            client_dss[i].id            = id_counter++;
            client_dss[i].next_instance = 0;
            client_dss[i].shared        = 0;
            return client_dss[i].id;
        }
    }
    return -1;
}

int get_data_index(int id)
{
    int i;
    for (i = 0; i < MAX_CLIENT_DSS; i++)
    {
        if (client_dss[i].usage_count > 0)
        {
            if (client_dss[i].id == id)
                return i;
        }
    }
    return -1;
}

int get_data_for_name(const char * name)
{
    int i;
    for (i = 0; i < MAX_CLIENT_DSS; i++)
    {
        if (client_dss[i].usage_count > 0)
        {
            if (strcmp(client_dss[i].name, name) == 0)
                return i;
        }
    }
    return -1;
}
