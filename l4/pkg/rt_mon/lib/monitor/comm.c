/**
 * \file   rt_mon/lib/monitor/comm.c
 * \brief  Some helper functions for communication with the coordinator.
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
#include <l4/rt_mon/monitor.h>

static l4_threadid_t server = L4_INVALID_ID;

static l4_threadid_t _get_threadid(void)
{
    int ret;
    l4_threadid_t id;

    ret = names_waitfor_name(RT_MON_COORD_NAME, &id, 10000);
    if (ret == 0)
        Panic(RT_MON_COORD_NAME" not found at names");
    return id;
}

int rt_mon_request_ds(int id, l4dm_dataspace_t * ds)
{
    int ret;
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    if (l4_is_invalid_id(server))
        server = _get_threadid();

    ret = rt_mon_coord_request_ds_call(&server, id, ds, &_dice_corba_env);
    if (ret)
    {
        LOG("Error requesting ds: %d", ret);
    }
    return ret;
}

int rt_mon_release_ds(int id)
{
    int ret;
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    if (l4_is_invalid_id(server))
        server = _get_threadid();

    ret = rt_mon_coord_release_ds_call(&server, id, &_dice_corba_env);
    if (ret)
    {
        LOG("Error releasing ds: %d", ret);
    }
    return ret;
}


int rt_mon_list_ds(rt_mon_dss ** dss, int count)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    if (l4_is_invalid_id(server))
        server = _get_threadid();

    rt_mon_coord_list_ds_call(&server, dss, &count, &_dice_corba_env);

    return count;
}
