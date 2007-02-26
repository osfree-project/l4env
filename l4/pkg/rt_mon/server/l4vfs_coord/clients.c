/**
 * \file   rt_mon/server/l4vfs_coord/clients.c
 * \brief  Client handling.
 *
 * \date   10/27/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include "clients.h"

client_t clients[MAX_CLIENTS];

int  client_get_free(void)
{
    int i;

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].open == 0)
            return i;
    }
    return -1;
}

int  client_is_open(int handle)
{
    return clients[handle].open;
}

void init_clients(void)
{
    int i;

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].open = 0;
    }
}
