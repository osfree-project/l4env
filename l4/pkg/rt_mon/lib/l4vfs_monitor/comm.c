/**
 * \file   rt_mon/lib/l4vfs_monitor/comm.c
 * \brief  Some helper functions for communication with the coordinator.
 *
 * \date   10/26/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdlib.h>
#include <errno.h>

#include <l4/dm_phys/dm_phys.h>
#include <l4/log/l4log.h>
//#include <l4/names/libnames.h>
#include <l4/sys/types.h>
//#include <l4/util/macros.h>

#include <l4/rt_mon/l4vfs_rt_mon-client.h>
#include <l4/rt_mon/types.h>
#include <l4/rt_mon/l4vfs_monitor.h>

#include <l4/l4vfs/types.h>
#include <l4/l4vfs/basic_name_server.h>
#include <l4/l4vfs/file-table.h>
#include <l4/l4vfs/io.h>
#include <l4/l4vfs/volumes.h>

int rt_mon_request_ds(void ** p, const char * name)
{
    int local_fd, ret;
    file_desc_t fd_s;
    object_handle_t object_handle;
    object_id_t object_id;
    l4_threadid_t server = L4_INVALID_ID;
    l4dm_dataspace_t ds;
    l4_size_t size;

    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    /* 1. try to resolve name
     * 2. get server for volume
     * 3. call server to request ds
     * 4. map locally
     * 5. create and return fd
     */

    local_fd = ft_get_next_free_entry();
    if (local_fd == -1)
    {
        errno = EMFILE;
        return -1;
    }

    object_id = l4vfs_resolve(l4vfs_name_server, cwd, name);

    // check for error
    if ((object_id.volume_id == L4VFS_ILLEGAL_VOLUME_ID) ||
        (object_id.object_id == L4VFS_ILLEGAL_OBJECT_ID))
    {
        // no file found
        errno = ENOENT;
        return -1;
    }

    // 2.
    ret = vol_resolve_thread_for_volume_id(object_id.volume_id, &server);
    if (ret)
    {
        errno = ret;
        return -1;
    }

    // 3.
    object_handle = rt_mon_l4vfs_coord_request_ds_call(&server, &object_id,
                                                       &ds, &_dice_corba_env);
    if (object_handle < 0)
    {
        LOG("Error requesting ds: %d", object_handle);
        errno = -object_handle;
        return -1;
    }

    // 4.
    ret = l4dm_mem_size(&ds, &size);
    if (ret)
    {
        LOG("Could not get size for dataspace!");
        // fixme: we should free the ds again
        errno = EACCES;
        return -1;
    }

    ret = l4rm_attach(&ds, size, 0, L4DM_RW, p);
    if (ret)
    {
        LOG("Could not attach dataspace!");
        errno = EACCES;
        return -1;
    }
    fd_s.user_data = *p;

    // 5.
    fd_s.server_id     = server;
    fd_s.object_handle = object_handle;
    fd_s.object_id     = object_id;

    // now finally copy acquired data to local table and return
    ft_fill_entry(local_fd, fd_s);
    return local_fd;
}

int rt_mon_release_ds(int fd)
{
    l4_int32_t ret;
    file_desc_t file_desc;

    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    if (! ft_is_open(fd))
    {
        errno = EBADF;
        return -1;
    }

    file_desc = ft_get_entry(fd);
    ret = l4rm_detach(file_desc.user_data);
    if (ret)
    {
        LOG("Could not unmap ds, ret = %d, ignored!", ret);
    }
    ret = rt_mon_l4vfs_coord_release_ds_call(&file_desc.server_id,
                                             file_desc.object_handle,
                                             &_dice_corba_env);
    if (ret != 0)
    {
        if (ret == EBADF)
        {
            // we got an inconsistant state to server
            errno = EIO;
            return -1;
        }
        else
        {
            // unknown case, what should I do?
            LOG("Error in close, unknown case, ret = %d!", ret);
            errno = -ret;
            return -1;
        }
    }

    ft_free_entry(fd);
    return 0;
}
