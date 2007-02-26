/**
 * \file   libc_backends_l4env/lib/mmap_util/mmap_util.c
 * \brief  Manages mappings between locally mapped dataspaces and the
 *         mapping servers
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdlib.h>

#include <l4/crtx/ctor.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/env/errno.h>
//#include <l4/log/l4log.h>
#include <l4/sys/types.h>

#include <l4/libc_backends_l4env/mmap_util.h>

#define MAX_DS2SERVER 4096

static ds2server_t ds2server_tbl[MAX_DS2SERVER];

int add_ds2server(l4dm_dataspace_t *ds, l4_threadid_t id,  l4_uint32_t area_id)
{
    int i;

    // sanity checks
    if (! ds || l4_is_invalid_id(id))
    {
        return -1;
    }

    for (i = 0; i < MAX_DS2SERVER; i++)
    {
        if (l4dm_is_invalid_ds(ds2server_tbl[i].ds))
        {
            ds2server_tbl[i].id      = id;
            ds2server_tbl[i].ds      = *ds;
            ds2server_tbl[i].area_id = area_id;
            return 0;
        }
    }

    return -1;
}

ds2server_t * get_ds2server(l4dm_dataspace_t *ds)
{
    int i;

    if (! ds)
        return NULL;

    for (i = 0; i < MAX_DS2SERVER; i++)
        if (ds2server_tbl[i].ds.id == ds->id)
            return &(ds2server_tbl[i]);

    return NULL;
}

int del_ds2server(l4dm_dataspace_t *ds)
{
    int i;

    if (! ds)
        return -1;

    for (i = 0; i < MAX_DS2SERVER; i++)
    {
        if (ds2server_tbl[i].ds.id == ds->id)
        {
            // we don't really need this cleanup, saves some cycles
            /*
            ds2server_tbl[i].id      = L4_INVALID_ID;
            ds2server_tbl[i].area_id = 0;
            */
            ds2server_tbl[i].ds      = L4DM_INVALID_DATASPACE;
            return 0;
        }
    }

    return -1;
}

void init_ds2server(void)
{
    int i;

    for (i = 0; i < MAX_DS2SERVER; i++)
    {
        ds2server_tbl[i].ds = L4DM_INVALID_DATASPACE;
    }
}
L4C_CTOR(init_ds2server, 1002);
