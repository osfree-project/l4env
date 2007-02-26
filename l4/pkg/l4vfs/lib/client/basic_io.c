/**
 * \file   l4vfs/lib/client/basic_io.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/l4vfs/basic_io.h>
#include <l4/l4vfs/basic_io-client.h>

#include <l4/sys/types.h>

#include <stdlib.h>

object_handle_t l4vfs_open(l4_threadid_t server,
                           object_id_t object_id,
                           l4_int32_t flags)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_basic_io_open_call(&server,
                                    &object_id,
                                    flags,
                                    &_dice_corba_env);
}

object_handle_t l4vfs_creat(l4_threadid_t server,
                            object_id_t parent,
                            const char* name,
                            l4_int32_t flags,
                            mode_t mode)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_basic_io_creat_call(&server,
                                     &parent,
                                     name,
                                     flags,
                                     mode,
                                     &_dice_corba_env);
}

off_t l4vfs_lseek(l4_threadid_t server,
                  object_handle_t fd,
                  off_t offset,
                  l4_int32_t whence)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_basic_io_lseek_call(&server,
                                     fd,
                                     offset,
                                     whence,
                                     &_dice_corba_env);
}

l4_int32_t l4vfs_fsync(l4_threadid_t server,
                       object_handle_t fd)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_basic_io_fsync_call(&server,
                                     fd,
                                     &_dice_corba_env);
}

l4_int32_t l4vfs_getdents(l4_threadid_t server,
                          object_handle_t fd,
                          l4vfs_dirent_t *dirp,
                          l4_uint32_t count)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_basic_io_getdents_call(&server,
                                        fd,
                                        &dirp,
                                        &count,
                                        &_dice_corba_env);
}

l4_int32_t l4vfs_stat(l4_threadid_t server,
                      object_id_t object_id,
                      l4vfs_stat_t * buf)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_basic_io_stat_call(&server,
                                    &object_id,
                                    buf,
                                    &_dice_corba_env);
}

l4_int32_t l4vfs_access(l4_threadid_t server,
                        object_id_t object_id,
                        l4_int32_t mode)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_basic_io_access_call(&server,
                                      &object_id,
                                      mode,
                                      &_dice_corba_env);
}

l4_int32_t l4vfs_unlink(l4_threadid_t server,
                        object_id_t object_id)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_basic_io_unlink_call(&server,
                                      &object_id,
                                      &_dice_corba_env);
}
