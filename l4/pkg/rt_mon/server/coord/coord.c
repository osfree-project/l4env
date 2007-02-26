/**
 * \file   rt_mon/server/coord/coord.c
 * \brief  Coordination server.
 *
 * \date   08/20/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/dm_generic/dm_generic.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/util/l4_macros.h>
#include <l4/util/parse_cmd.h>

#include <l4/rt_mon/rt_mon-server.h>

#include <stdlib.h>

#include "coord.h"
#include "clients.h"

char LOG_tag[9] = RT_MON_COORD_NAME;
static int verbose;

#define MIN ((a)<(b)?(a):(b))

void check_and_free(int index);
void check_and_free(int index)
{
    if (client_dss[index].usage_count <= 0)
    {
        int ret = l4dm_close(&(client_dss[index].ds));
        if (ret)
        {
            LOG("Problem closing dataspace: ret = %d", ret);
        }
    }
}

l4_int32_t
rt_mon_reg_register_ds_component(CORBA_Object _dice_corba_obj,
                                 const l4dm_dataspace_t *ds,
                                 const char* name,
                                 CORBA_Server_Environment *_dice_corba_env)
{
    int id, ret;

    if (name == NULL)
        return -1;

    id = clients_insert_data(ds, name);
    LOGd(verbose, "Registered with id = %d.", id);
    ret = l4dm_share(ds, *_dice_corba_obj, L4DM_RW);
    if (ret)
    {
        LOG("Cannot share access rights for ds: ret = %d", ret);
        return -1;
    }
    return id;
}

l4_int32_t
rt_mon_reg_request_shared_ds_component(
    CORBA_Object _dice_corba_obj,
    l4dm_dataspace_t *ds,
    l4_int32_t length,
    const char* name,
    l4_int32_t *instance,
    CORBA_Server_Environment *_dice_corba_env)
{
    /* 1. check if name exists allready? -> return this on, else ...
     * 2. create new ds with desired size, map it locally
     * 3. store information locally  |  call *register* for both together
     * 4. share dataspace to client  |
     * 5. set type information in shared dataspace to illegal value, unmap
     */

    int ret, index, id;
    rt_mon_data * temp;

    if (name == NULL)
        return -1;

    // 1.
    index = get_data_for_name(name);
    if (index >= 0)
    {
        if (client_dss[index].shared == 0)
        {
            LOG("Attempt to access a non-shared sensor in shared mode"
                ", ignored!");
            return -1;
        }
        ret = l4dm_share(&(client_dss[index].ds), *_dice_corba_obj, L4DM_RW);
        if (ret)
        {
            LOG("Cannot share access rights for ds: ret = %d", ret);
            return -1;
        }

        client_dss[index].usage_count++;
        *instance = client_dss[index].next_instance++;
        *ds       = client_dss[index].ds;

        return client_dss[index].id;
    }

    // 2.
    temp = l4dm_mem_ds_allocate_named(length, L4DM_PINNED | L4RM_MAP,
                                      name, ds);
    if (temp == NULL)
        return -1;

    // 3. + 4.
    id = rt_mon_reg_register_ds_component(_dice_corba_obj, ds, name,
                                          _dice_corba_env);
    if (id < 0)
    {
        l4rm_detach(temp);
        return -1;
    }
    index = get_data_index(id);
    if (index < 0)
    {
        LOG("Oops.");
        l4rm_detach(temp);
        return -1;
    }
    client_dss[index].shared = 1;

    // 5.
    temp->type = RT_MON_TYPE_UNDEFINED;
    l4rm_detach(temp);
    *instance = client_dss[index].next_instance++;

    return id;
}

l4_int32_t
rt_mon_reg_unregister_ds_component(CORBA_Object _dice_corba_obj,
                                   l4_int32_t id,
                                   CORBA_Server_Environment *_dice_corba_env)
{
    int ret, i;

    LOGd(verbose, "id=%d", id);
    i = get_data_index(id);
    if (i < 0)
    {
        LOG("DS with id = %d not found", id);
        return -1;
    }

    // revoke all the rights at first
    ret = l4dm_revoke(&(client_dss[i].ds), *_dice_corba_obj, L4DM_RW);
    if (ret)
    {
        LOG("Cannot revoke access rights for ds: ret = %d", ret);
        return -1;
    }

    client_dss[i].usage_count--;
    check_and_free(i);

    return 0;
}

l4_int32_t
rt_mon_coord_request_ds_component(CORBA_Object _dice_corba_obj,
                                  l4_int32_t id,
                                  l4dm_dataspace_t *ds,
                                  CORBA_Server_Environment *_dice_corba_env)
{
    int i;

    // error checks
    if (id < 0)
        return -1;

    i = get_data_index(id);
    if (i < 0)
        return -1;  // not found

    client_dss[i].usage_count++;

    // copy data and give rights
    *ds = client_dss[i].ds;
    l4dm_share(ds, *_dice_corba_obj, L4DM_RW);

    return 0;
}

l4_int32_t
rt_mon_coord_release_ds_component(CORBA_Object _dice_corba_obj,
                                  l4_int32_t id,
                                  CORBA_Server_Environment *_dice_corba_env)
{
    int i, ret;

    // error checks
    if (id < 0)
        return -1;

    i = get_data_index(id);
    if (i < 0)
        return -1;  // not found

    LOGd(verbose, "DS: id = %d, manager = "l4util_idfmt"",
        client_dss[i].ds.id, l4util_idstr(client_dss[i].ds.manager));
    ret = l4dm_revoke(&(client_dss[i].ds), *_dice_corba_obj, L4DM_ALL_RIGHTS);
    if (ret)
    {
        LOG("Cannot revoke access rights for ds: ret = %d", ret);
        //return -1;
    }
    client_dss[i].usage_count--;
    check_and_free(i);

    return 0;
}

void
rt_mon_coord_list_ds_component(CORBA_Object _dice_corba_obj,
                               rt_mon_dss *dss[],
                               l4_int32_t *count,
                               CORBA_Server_Environment *_dice_corba_env)
{
    int i, j;

    for(i = 0, j = 0; i < MAX_CLIENT_DSS && j < *count; i++)
    {
        // skip empty entries
        if (client_dss[i].usage_count <= 0)
            continue;

        // copy one entry
        (*dss)[j].id = client_dss[i].id;
        strncpy((*dss)[j].name, client_dss[i].name, RT_MON_NAME_LENGTH);
        j++;
    }

    *count = j;
}

int main(int argc, const char *argv[])
{
    CORBA_Server_Environment env = dice_default_server_environment;

    if (parse_cmdline(&argc, &argv,
                      'v', "verbose", "log requests",
                      PARSE_CMD_SWITCH, 1, &verbose, 0, 0) != 0)
    {
        return 1;
    }

    clients_init_data();

    names_register(RT_MON_COORD_NAME);

    env.malloc = (dice_malloc_func)malloc;
    env.free = (dice_free_func)free;
    rt_mon_coord_server_loop(&env);
}
