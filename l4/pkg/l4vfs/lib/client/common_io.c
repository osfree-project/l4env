/**
 * \file   l4vfs/lib/client/common_io.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/l4vfs/common_io.h>
#include <l4/l4vfs/common_io-client.h>
#include <l4/sys/types.h>

#include <stdlib.h>

ssize_t l4vfs_read(l4_threadid_t server,
                   object_handle_t fd,
                   l4_int8_t **buf,
                   size_t *count)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_common_io_read_call(&server,
                                     fd,
                                     buf,
                                     count,
                                     &_dice_corba_env);
}

ssize_t l4vfs_write(l4_threadid_t server,
                    object_handle_t fd,
                    const l4_int8_t *buf,
                    size_t *count)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_common_io_write_call(&server,
                                      fd,
                                      buf,
                                      count,
                                      &_dice_corba_env);
}

int l4vfs_close(l4_threadid_t server, object_handle_t object_handle)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_common_io_close_call(&server,
                                      object_handle,
                                      &_dice_corba_env);
}

int l4vfs_ioctl(l4_threadid_t server,
                object_handle_t fd,
                l4_int32_t cmd,
                char **arg,
                size_t *count)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_common_io_ioctl_call( &server,
                                       fd,
                                       cmd,
                                       (l4_int8_t **) arg,
                                       count,
                                       &_dice_corba_env );
}

int l4vfs_fcntl(l4_threadid_t server,
                object_handle_t fd,
                l4_int32_t cmd,
                l4_int32_t *arg)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_common_io_fcntl_call(&server,
                                      fd,
                                      cmd,
                                      arg,
                                      &_dice_corba_env);
}
