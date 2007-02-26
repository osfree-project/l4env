/**
 * \file   l4vfs/l4vfs_log/server/main.c
 * \brief  
 *
 * \date   11/05/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/util/util.h>


#include <l4/l4vfs/basic_io.h>
#include <l4/l4vfs/basic_name_server.h>
#include <l4/l4vfs/name_space_provider.h>
#include <l4/l4vfs/name_server.h>
#include <l4/l4vfs/object_server-server.h>
#include <l4/l4vfs/tree_helper.h>
#include <l4/l4vfs/types.h>

#include "clients.h"

#define L4VFS_LOG_TAG "l4vfs_log"


char LOG_tag[9] = L4VFS_LOG_TAG;
l4vfs_th_node_t * root;


object_handle_t
l4vfs_basic_io_open_component(CORBA_Object _dice_corba_obj,
                              const object_id_t *object_id,
                              l4_int32_t flags,
                              CORBA_Server_Environment *_dice_corba_env)
{
    /* 1. check for valid object_id
     * 2. open it: create clientstate
     * 3. return handle
     */
    l4vfs_th_node_t * node;
    int handle;

    // 1.
    if (object_id->volume_id != L4VFS_LOG_VOLUME_ID)
        return -ENOENT;

    node = l4vfs_th_node_for_id(object_id->object_id);
    if (node == NULL)
        return -ENOENT;

    LOG("%x, %x", flags, flags & O_ACCMODE);
    if (node->type == L4VFS_TH_TYPE_OBJECT)
    {
        if ((flags & O_ACCMODE) != O_WRONLY)
            return -EACCES;
    }
    else if (node->type == L4VFS_TH_TYPE_DIRECTORY)
    {
        if ((flags & O_ACCMODE) != O_RDONLY)
            return -EACCES;
    }

    handle = client_get_free();
    if (handle < 0)
        return -ENOMEM;

    clients[handle].open   = 1;
    clients[handle].client = *_dice_corba_obj;
    clients[handle].node   = node;
    clients[handle].seek   = 0;
    node->usage_count++;
    return handle;
}

l4_int32_t
l4vfs_common_io_close_component(CORBA_Object _dice_corba_obj,
                                object_handle_t handle,
                                CORBA_Server_Environment *_dice_corba_env)
{
    /* 1. check for valid object_handle, check owner
     * 2. flush and close
     * 3. check usage count and remove in case == 0
     */
    int ret;
    l4vfs_th_node_t * node;

    // 1.
    if (handle < 0 || handle >= MAX_CLIENTS)
        return -EBADF;
    if (! client_is_open(handle))
        return -EBADF;
    if (! l4_task_equal(clients[handle].client, *_dice_corba_obj))
        return -EBADF;

    // 2.
    LOG_flush();
    clients[handle].open   = 0;
    node = clients[handle].node;
    node->usage_count--;

    // 3.
    if (node->usage_count <= 0 && node->type == L4VFS_TH_TYPE_OBJECT)
    {
        ret = l4vfs_th_destroy_child(node->parent, node);
        if (ret)
        {
            LOG("Problem freeing node: %p, ignored", node);
        }
    }
    clients[handle].node = NULL;  // just in case ...

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
                                object_handle_t handle,
                                const l4_int8_t *buf,
                                l4vfs_size_t *count,
                                l4_int16_t *_dice_reply,
                                CORBA_Server_Environment *_dice_corba_env)
{
    /* 1. check for valid object_handle, check owner
     * 2. configure log with prefix
     * 3. output string & restore prefix
     */

    // 1.
    if (handle < 0 || handle >= MAX_CLIENTS)
        return -EBADF;
    if (! client_is_open(handle))
        return -EBADF;
    if (! l4_task_equal(clients[handle].client, *_dice_corba_obj))
        return -EBADF;

    // 2.
    strncpy(LOG_tag, clients[handle].node->name, 8);
    LOG_tag[9] = 0;

    // 3.
    LOG_printf("%.*s", *count, buf);
    strcpy(LOG_tag, L4VFS_LOG_TAG);
    return *count;
}

l4vfs_off_t
l4vfs_basic_io_lseek_component(CORBA_Object _dice_corba_obj,
                               object_handle_t handle,
                               l4vfs_off_t offset,
                               l4_int32_t whence,
                               CORBA_Server_Environment *_dice_corba_env)
{
    /* 1. check for valid handle
     * 2a. if file return ESPIPE
     * 2b. if dir ...
     */
    
    // 1.
    if (handle < 0 || handle >= MAX_CLIENTS)
        return -EBADF;
    if (! client_is_open(handle))
        return -EBADF;
    if (! l4_task_equal(clients[handle].client, *_dice_corba_obj))
        return -EBADF;

    // 2.
    if (clients[handle].node->type == L4VFS_TH_TYPE_OBJECT)
        return -ESPIPE;
    else
    {
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
    if (object_id->volume_id != L4VFS_LOG_VOLUME_ID)
        return -ENOENT;
    node = l4vfs_th_node_for_id(object_id->object_id);
    if (node == NULL)
        return -ENOENT;

    // 2.
    buf->st_dev   = L4VFS_LOG_VOLUME_ID;
    buf->st_ino   = object_id->object_id;
    if (node->type == L4VFS_TH_TYPE_OBJECT)
        buf->st_mode = S_IFREG;
    else
        buf->st_mode  = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
    buf->st_nlink = 1;
    // fixme: other fields are undefined for now ...

    return 0;
}

object_id_t
l4vfs_basic_name_server_resolve_component(
    CORBA_Object _dice_corba_obj,
    const object_id_t *base,
    const char* pathname,
    CORBA_Server_Environment *_dice_corba_env)
{
    /* Trace all resolve requests and dynamically create nodes accordingly.
     *
     * If there are no node spaces left, delete the oldest not-open
     * one and replace it with the current request.
     */

    l4vfs_th_node_t * node;
    char * p;
    object_id_t ret;

    LOG("pathname = '%s'", pathname);
    if (strcmp(".", pathname) == 0 ||
        strcmp("/", pathname) == 0 ||
        strlen(pathname) == 0)
    {
        ret.object_id = root->id;
        ret.volume_id = L4VFS_LOG_VOLUME_ID;
        return ret;
    }

    // check for illegal characters
    p = strpbrk(pathname, L4VFS_ILLEGAL_OBJECT_NAME_CHARS);
    if (p != NULL)
    {
        ret.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
        ret.object_id = L4VFS_ILLEGAL_OBJECT_ID;
        return ret;
    }

    node = l4vfs_th_child_for_name(root, pathname);

    if (node == NULL)
    {
        node = l4vfs_th_new_node(pathname, L4VFS_TH_TYPE_OBJECT, root, NULL);
        if (node == NULL)  // could not create node, out of memory?
        {
            // fixme: we could remove some old unused nodes here and try again!
            ret.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
            ret.object_id = L4VFS_ILLEGAL_OBJECT_ID;
            return ret;
        }
    }

    ret.object_id = node->id;
    ret.volume_id = L4VFS_LOG_VOLUME_ID;
    return ret;
}

l4_threadid_t
l4vfs_basic_name_server_thread_for_volume_component(
    CORBA_Object _dice_corba_obj,
    volume_id_t volume_id,
    CORBA_Server_Environment *_dice_corba_env)
{
    if (volume_id == L4VFS_LOG_VOLUME_ID)
    {
        return l4_myself();
    }
    return L4_INVALID_ID;
}


int main(int argc, char *argv[])
{
    l4_threadid_t ns;
    CORBA_Server_Environment env = dice_default_server_environment;
    object_id_t root_id;

    l4vfs_th_init();
    root = l4vfs_th_new_node(L4VFS_LOG_TAG, L4VFS_TH_TYPE_DIRECTORY,
                             NULL, NULL);

    l4_sleep(500);
    ns = l4vfs_get_name_server_threadid();
    root_id.volume_id = L4VFS_LOG_VOLUME_ID;
    root_id.object_id = root->id;
    l4vfs_register_volume(ns, l4_myself(), root_id);
    LOG("Registered my volume at name_server");

    env.malloc = (dice_malloc_func)malloc;
    env.free = (dice_free_func)free;
    l4vfs_object_server_server_loop(&env);

    return 0;
}
