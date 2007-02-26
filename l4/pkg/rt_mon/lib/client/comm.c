/**
 * \file   rt_mon/lib/client/comm.c
 * \brief  Functions to access coordinator from client applications.
 *
 * \date   08/20/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdlib.h>

#include <l4/dm_phys/dm_phys.h>
#include <l4/names/libnames.h>
#include <l4/sys/types.h>
#include <l4/util/macros.h>

#include <l4/rt_mon/rt_mon-client.h>
#include <l4/rt_mon/types.h>

#include "comm.h"

static l4_threadid_t server = L4_INVALID_ID;

static l4_threadid_t rt_mon_comm_get_threadid(void)
{
    int ret;
    l4_threadid_t id;

    ret = names_waitfor_name(RT_MON_COORD_NAME, &id, 10000);
    if (ret == 0)
        Panic(RT_MON_COORD_NAME" not found at names");
    return id;
}

int rt_mon_register_ds(l4dm_dataspace_t ds, const char * name)
{
    int ret;
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    if (l4_is_invalid_id(server))
        server = rt_mon_comm_get_threadid();

    // give right away, coord will share right back
    ret = l4dm_transfer(&ds, server);
    if (ret)
    {
        LOG_Error("Cannot transfer dataspace ownership for ds: ret = %d", ret);
        return -1;
    }
    ret = rt_mon_reg_register_ds_call(&server, &ds, name, &_dice_corba_env);
    if (ret < 0)
    {
        LOG_Error("Registering ds: %d", ret);
    }

    return ret;
}

int rt_mon_unregister_ds(int id)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    if (l4_is_invalid_id(server))
        server = rt_mon_comm_get_threadid();

    return rt_mon_reg_unregister_ds_call(&server, id, &_dice_corba_env);
}

int rt_mon_request_shared_ds(l4dm_dataspace_t * ds, int length,
                             const char * name, int * instance, void ** p)
{
    /* 1. go and request the dataspace
     * 2. map its content into local address space and return pointer in p
     */

    int ret, id;
    l4_size_t size;

    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    if (l4_is_invalid_id(server))
        server = rt_mon_comm_get_threadid();

    id = rt_mon_reg_request_shared_ds_call(&server, ds, length, name, instance,
                                           &_dice_corba_env);
    if (id < 0)
    {
        LOG("Error requesting ds: %d", id);
        return id;
    }
    ret = l4dm_mem_size(ds, &size);
    if (ret)
    {
        LOG("Could not get size for dataspace!");
        return -1;
    }

    ret = l4rm_attach(ds, size, 0, L4DM_RW | L4RM_MAP, p);
    if (ret)
    {
        LOG("Could not attach dataspace!");
        return -1;
    }

    return id;
}
