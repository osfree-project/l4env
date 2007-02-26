/**
 * \file   rt_mon/server/l4vfs_coord/coord.c
 * \brief  Coordination server with l4vfs interface.
 *
 * \date   10/26/2004
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
#include <l4/sys/syscalls.h>
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>

#include <l4/rt_mon/l4vfs_rt_mon-server.h>

#include <l4/l4vfs/name_server.h>
#include <l4/l4vfs/name_space_provider.h>
#include <l4/l4vfs/tree_helper.h>

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "clients.h"
#include "coord.h"
#include "helper.h"

#define MIN ((a)<(b)?(a):(b))

char LOG_tag[9] = RT_MON_COORD_NAME;
static l4vfs_th_node_t * root;


l4_int32_t
rt_mon_reg_register_ds_component(CORBA_Object _dice_corba_obj,
                                 const l4dm_dataspace_t *ds,
                                 const char* name,
                                 CORBA_Server_Environment *_dice_corba_env)
{
    /* 1. Split name
     * 2. For each chunk check, whether such a dir exists
     *     - if not -> create it
     * 3. check remaining name
     *     - yes, it exists -> return error
     *     - no -> create new one, set usage count to 1, share ds back,
     *             return ok
     */

    char * remainder;
    int id = L4VFS_ROOT_OBJECT_ID, ret;

    if (name == NULL)
        return -1;

    LOG("register: '%s'", name);
    if (l4vfs_th_is_absolute_path(name))
    {
        remainder = l4vfs_th_get_remainder_path(name);
    }
    else // normal case
    {
        remainder = strdup(name);
    }

    while (strlen(remainder) > 0)
    {
        char * path, * temp, flag;
        l4vfs_th_node_t * node, * parent;

        path = l4vfs_th_get_first_path(remainder);
        temp = l4vfs_th_get_remainder_path(remainder);
        free(remainder);
        remainder = temp;

        do
        {
            flag = 0;
            ret = l4vfs_th_local_resolve(id, path);
            if (ret != L4VFS_ILLEGAL_OBJECT_ID && strlen(remainder) == 0)
            {  // existing file found -> we have to version the name
                flag = 1;
                path = _version_name(path);
            }
        }
        while (flag == 1);

        if (ret == L4VFS_ILLEGAL_OBJECT_ID && strlen(remainder) == 0)
        {  // nothing found & we tested for a file -> create file
            file_t * file;

            LOG("register: '%s'", path);

            file = malloc(sizeof(file_t));
            file->ds            = *ds;
            file->next_instance = 0;
            file->shared        = 0;
            parent = l4vfs_th_node_for_id(id);
            node = l4vfs_th_new_node(path, L4VFS_TH_TYPE_OBJECT, parent,
                                     file);
            node->usage_count   = 1;
            id = node->id;
        }
        else if (ret == L4VFS_ILLEGAL_OBJECT_ID && strlen(remainder) != 0)
        {  // nothing found & we tested for a dir -> create dir and dive in
            parent = l4vfs_th_node_for_id(id);
            node = l4vfs_th_new_node(path, L4VFS_TH_TYPE_DIRECTORY, parent,
                                     NULL);
            id = node->id;
        }
        else if (ret != L4VFS_ILLEGAL_OBJECT_ID && strlen(remainder) != 0)
        {  // existing dir found -> dive into it
            id = ret;
        }
        else
        {  // existing file found -> bad, no new register possible
            LOG("File already exists!");
            free(remainder);
            free(path);
            return -1;
        }
        free(path);
    }
    free(remainder);

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
    /* 1. check if name exists already? -> return this on, else ...
     * 2. create new ds with desired size, map it locally
     * 3. store information locally  |  call *register* for both together
     * 4. share dataspace to client  |
     * 5. set type information in shared dataspace to illegal value, unmap
     */

    int ret;
    rt_mon_data * temp;
    local_object_id_t oid, base = L4VFS_ROOT_OBJECT_ID;
    l4vfs_th_node_t * node;
    file_t * file;

    if (name == NULL)
        return -1;

    // 1.
    oid = l4vfs_th_resolve(base, name);
    if (oid != L4VFS_ILLEGAL_OBJECT_ID)
    {  // found something

        node = l4vfs_th_node_for_id(oid);
        if (node == NULL)
        {
            LOG("How could this happen?");
            return -1;
        }
        file = (file_t *)node->data;
        if (file->shared == 0)
        {
            LOG("Attempt to access a non-shared sensor in shared mode"
                ", ignored!");
            return -1;
        }
        ret = l4dm_share(&(file->ds), *_dice_corba_obj, L4DM_RW);
        if (ret)
        {
            LOG("Cannot share access rights for ds: ret = %d", ret);
            return -1;
        }
        *ds = file->ds;
        *instance = file->next_instance++;
        node->usage_count++;

        LOG("id = %d", oid);
        return oid;
    }

    // 2.
    temp = l4dm_mem_ds_allocate_named(length, L4DM_PINNED | L4RM_MAP,
                                      name, ds);
    if (temp == NULL)
        return -1;

    // 3. + 4.
    oid = rt_mon_reg_register_ds_component(_dice_corba_obj, ds, name,
                                          _dice_corba_env);
    if (oid < 0)
    {
        l4rm_detach(temp);
        return -1;
    }
    node = l4vfs_th_node_for_id(oid);
    if (node == NULL)
    {
        LOG("How could this happen?");
        return -1;
    }
    file = (file_t *)node->data;
    file->shared = 1;

    // 5.
    temp->type = RT_MON_TYPE_UNDEFINED;
    l4rm_detach(temp);
    *instance = file->next_instance++;

    return oid;
}

l4_int32_t
rt_mon_reg_unregister_ds_component(CORBA_Object _dice_corba_obj,
                                   l4_int32_t id,
                                   CORBA_Server_Environment *_dice_corba_env)
{
    /* 1. check if file exists.
     * 2. decrease usage count, unshare ds
     * 3. check usage count if == 0 -> remove file & maybe parent dir(s)
     */
    int ret;
    l4vfs_th_node_t * node;
    file_t * file;

    LOG("Unregister Component");
    node = l4vfs_th_node_for_id(id);
    if (node == NULL)
    {
        return -1;
    }

    file = (file_t *)node->data;

    // revoke all the rights at first
    ret = l4dm_revoke(&(file->ds), *_dice_corba_obj, L4DM_RW);
    if (ret)
    {
        LOG("Cannot revoke access rights for ds: ret = %d", ret);
        return -1;
    }

    node->usage_count--;
    if (node->usage_count <= 0)
    {
        l4dm_close(&(file->ds));
        _check_and_clean_node(node);
    }

    return 0;
}

object_handle_t
rt_mon_l4vfs_coord_request_ds_component(
    CORBA_Object _dice_corba_obj,
    const object_id_t *id,
    l4dm_dataspace_t *ds,
    CORBA_Server_Environment *_dice_corba_env)
{
    /* 1. check if valid id
     * 2. increase usage count, share ds, create client state
     * 3. return client state & ds
     */

    l4vfs_th_node_t * node;
    object_handle_t handle;
    file_t * file;

    // 1.
    if (id->volume_id != RT_MON_L4VFS_COORD_VOLUME_ID)
        return -ENOENT;
    node = l4vfs_th_node_for_id(id->object_id);
    if (node == NULL)
        return -ENOENT;
    if (node->type != L4VFS_TH_TYPE_OBJECT)
        return -EINVAL;

    // 2.
    handle = client_get_free();
    if (handle < 0)
        return ENOMEM;
    file = (file_t *)node->data;
    clients[handle].open   = 1;
    clients[handle].mode   = 0;
    clients[handle].seek   = 0;
    clients[handle].client = *_dice_corba_obj;
    clients[handle].node   = node;
    node->usage_count++;
    l4dm_share(&(file->ds), *_dice_corba_obj, L4DM_RW);

    // 3.
    *ds = file->ds;
    return handle;
}

l4_int32_t
rt_mon_l4vfs_coord_release_ds_component(
    CORBA_Object _dice_corba_obj,
    object_handle_t handle,
    CORBA_Server_Environment *_dice_corba_env)
{
    /* 1. check if valid id
     * 2. decrease usage count, unshare ds, remove client state
     * 3. check if usage count == 0 -> remove file & maybe parent dir(s)
     * 4. return ok
     */

    int ret;
    file_t * file;
    l4vfs_th_node_t * node;

    // 1.
    if (handle < 0 || handle >= MAX_CLIENTS)
        return -EBADF;
    if (! client_is_open(handle))
        return -EBADF;
    if (! l4_task_equal(clients[handle].client, *_dice_corba_obj))
        return -EBADF;
    if (clients[handle].node->type != L4VFS_TH_TYPE_OBJECT)
        return -EBADF;

    // 2.
    node = clients[handle].node;
    file = (file_t *)node->data;
    node->usage_count--;
    ret = l4dm_revoke(&(file->ds), *_dice_corba_obj, L4DM_RW);
    if (ret)
    {
        LOG("Cannot revoke access rights for ds: ret = %d, ignored for now!",
            ret);
    }
    clients[handle].open = 0;

    // 3.
    if (node->usage_count <= 0)
    {
        l4dm_close(&(file->ds));
        _check_and_clean_node(node);
    }

    // 4.
    return 0;
}

object_handle_t
l4vfs_basic_io_open_component(CORBA_Object _dice_corba_obj,
                              const object_id_t *object_id,
                              l4_int32_t flags,
                              CORBA_Server_Environment *_dice_corba_env)
{
    /* 1. check if valid id
     * 2. check of mode == RO
     * 3. check if dir
     * 4. create a new client state
     * 5. return handle
     */
    l4vfs_th_node_t * node;
    int handle;

    // 1.
    if (object_id->volume_id != RT_MON_L4VFS_COORD_VOLUME_ID)
        return -ENOENT;
    node = l4vfs_th_node_for_id(object_id->object_id);
    if (node == NULL)
        return -ENOENT;

    // 2.
    if ((flags & O_ACCMODE) != O_RDONLY)
        return -EROFS;

    // 3.
    if (node->type != L4VFS_TH_TYPE_DIRECTORY)
        return -EINVAL;

    // 4.
    handle = client_get_free();
    if (handle < 0)
        return ENOMEM;
    clients[handle].open   = 1;
    clients[handle].mode   = flags;
    clients[handle].seek   = 0;
    clients[handle].client = *_dice_corba_obj;
    clients[handle].node   = node;
    node->usage_count++;

    return handle;
}

l4_int32_t
l4vfs_common_io_close_component(CORBA_Object _dice_corba_obj,
                                object_handle_t handle,
                                CORBA_Server_Environment *_dice_corba_env)
{
    if (handle < 0 || handle >= MAX_CLIENTS)
        return -EBADF;
    if (! client_is_open(handle))
        return -EBADF;
    if (! l4_task_equal(clients[handle].client, *_dice_corba_obj))
        return -EBADF;
    if (clients[handle].node->type != L4VFS_TH_TYPE_DIRECTORY)
        return -EBADF;

    clients[handle].node->usage_count--;
    clients[handle].open = 0;

    if (clients[handle].node->usage_count <= 0)
    {
        _check_and_clean_node(clients[handle].node);
    }

    return 0;
}

l4_int32_t
l4vfs_basic_io_getdents_component(CORBA_Object _dice_corba_obj,
                                  object_handle_t handle,
                                  l4vfs_dirent_t *dirp,
                                  l4_uint32_t *count,
                                  CORBA_Server_Environment *_dice_corba_env)
{
    int ret, seek;
    l4vfs_th_node_t * node;

    if (handle < 0 || handle >= MAX_CLIENTS)
        return -EBADF;
    if (! client_is_open(handle))
        return -EBADF;
    if (! l4_task_equal(clients[handle].client, *_dice_corba_obj))
        return -EBADF;

    node = clients[handle].node;
    seek = clients[handle].seek;
    ret = l4vfs_th_dir_fill_dirents(node, seek, dirp, count);
    clients[handle].seek = ret;  // set new seekpointer

    if (*count < 0)
        return -EINVAL;
    else
        return *count;
}

l4_int32_t
l4vfs_basic_io_stat_component(CORBA_Object _dice_corba_obj,
                              const object_id_t *object_id,
                              l4vfs_stat_t *buf,
                              CORBA_Server_Environment *_dice_corba_env)
{
    /* 1. check if object exists
     * 2. fill data structure
     * 3. return it
     */
    l4vfs_th_node_t * node;

    // 1.
    if (object_id->volume_id != RT_MON_L4VFS_COORD_VOLUME_ID)
        return -ENOENT;
    node = l4vfs_th_node_for_id(object_id->object_id);
    if (node == NULL)
        return -ENOENT;

    // 2.
    buf->st_dev   = RT_MON_L4VFS_COORD_VOLUME_ID;
    buf->st_ino   = object_id->object_id;
    if (node->type == L4VFS_TH_TYPE_OBJECT)
        buf->st_mode = S_IFREG;
    else
        buf->st_mode  = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
    buf->st_nlink = 1;
    // fixme: other fields are undefined for now ...

    return 0;
}

l4vfs_off_t
l4vfs_basic_io_lseek_component(CORBA_Object _dice_corba_obj,
                               object_handle_t handle,
                               l4vfs_off_t offset,
                               l4_int32_t whence,
                               CORBA_Server_Environment *_dice_corba_env)
{
    if (handle < 0 || handle >= MAX_CLIENTS)
        return -EBADF;
    if (! client_is_open(handle))
        return -EBADF;
    if (! l4_task_equal(clients[handle].client, *_dice_corba_obj))
        return -EBADF;

    switch (whence)  // fixme: care for all the other cases ...
    {
    case SEEK_SET:
        if (offset != 0)
            return -EINVAL;
        clients[handle].seek = offset;
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

l4vfs_ssize_t
l4vfs_common_io_read_component(CORBA_Object _dice_corba_obj,
                               object_handle_t fd,
                               l4_int8_t **buf,
                               l4vfs_size_t *count,
                               l4_int16_t *_dice_reply,
                               CORBA_Server_Environment *_dice_corba_env)
{
    return -EINVAL;
}

l4vfs_ssize_t
l4vfs_common_io_write_component(CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                const l4_int8_t *buf,
                                l4vfs_size_t *count,
                                l4_int16_t *_dice_reply,
                                CORBA_Server_Environment *_dice_corba_env)
{
    return -EINVAL;
}

object_id_t
l4vfs_basic_name_server_resolve_component(
    CORBA_Object _dice_corba_obj,
    const object_id_t *base,
    const char* pathname,
    CORBA_Server_Environment *_dice_corba_env)
{
    local_object_id_t l_id;
    object_id_t id;

    if (base->volume_id != RT_MON_L4VFS_COORD_VOLUME_ID)
    {
        id.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
        id.object_id = L4VFS_ILLEGAL_OBJECT_ID;
        return id;
    }

    l_id = l4vfs_th_resolve(base->object_id, pathname);

    if (l_id == L4VFS_ILLEGAL_OBJECT_ID)
    {
        id.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
        id.object_id = L4VFS_ILLEGAL_OBJECT_ID;
    }
    else
    {
        id.volume_id = RT_MON_L4VFS_COORD_VOLUME_ID;
        id.object_id = l_id;
    }
    return id;
}


char*
l4vfs_basic_name_server_rev_resolve_component(
    CORBA_Object _dice_corba_obj,
    const object_id_t *dest,
    object_id_t *parent,
    CORBA_Server_Environment *_dice_corba_env)
{
    char * ret;

    if (dest->volume_id != RT_MON_L4VFS_COORD_VOLUME_ID ||
        parent->volume_id != RT_MON_L4VFS_COORD_VOLUME_ID)
    {
        return NULL;
    }

    ret = l4vfs_th_rev_resolve(dest->object_id, &(parent->object_id));

    // tell dice to free the pointer after the reply
    dice_set_ptr(_dice_corba_env, ret);

    return ret;
}

l4_threadid_t
l4vfs_basic_name_server_thread_for_volume_component(
    CORBA_Object _dice_corba_obj,
    volume_id_t volume_id,
    CORBA_Server_Environment *_dice_corba_env)
{
    if (volume_id == RT_MON_L4VFS_COORD_VOLUME_ID)
    {
        return l4_myself();
    }
    return L4_INVALID_ID;
}

int main(int argc, char *argv[])
{
    CORBA_Server_Environment env = dice_default_server_environment;
    l4_threadid_t ns;
    object_id_t root_id;
    int i, ret;

    names_register(RT_MON_COORD_NAME);

    // create root node
    root = l4vfs_th_new_node("[rt_mon root]", L4VFS_TH_TYPE_DIRECTORY,
                             NULL, NULL);

    ns = l4vfs_get_name_server_threadid();
    LOG("got name server thread_id");
    root_id.volume_id = RT_MON_L4VFS_COORD_VOLUME_ID;
    root_id.object_id = 0;

    for (i = 0; i < 3; i++)
    {
        ret = l4vfs_register_volume(ns, l4_myself(), root_id);
        // register ok --> done
        if (ret == 0)
            break;
        else
            LOG("error registering: %d", ret);
        l4_sleep(500);
    }
    if (ret)
        exit(1);

    env.malloc = (dice_malloc_func)malloc;
    env.free = (dice_free_func)free;
    rt_mon_l4vfs_coord_server_loop(&env);
}
