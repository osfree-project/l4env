/**
 * \file   l4vfs/lib/client/mmap_io.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdlib.h>
#include <errno.h>

#include <l4/l4vfs/mmap_io.h>
#include <l4/l4vfs/mmap-client.h>

#include <l4/sys/types.h>

l4_int32_t l4vfs_mmap1(l4_threadid_t server,
                       l4dm_dataspace_t *ds,
                       l4vfs_size_t length,
                       l4_int32_t prot,
                       l4_int32_t flags,
                       object_handle_t fd,
                       l4vfs_off_t offset)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_mmap_mmap_call(&server, ds, length, prot, flags, fd, offset,
                                &_dice_corba_env);
}

l4_int32_t l4vfs_msync(l4_threadid_t server,
                       l4dm_dataspace_t *ds,
                       l4_addr_t start,
                       l4vfs_size_t length,
                       l4_int32_t flags)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_mmap_msync_call(&server, ds, start, length, flags,
                                 &_dice_corba_env);
}

l4_int32_t l4vfs_munmap(l4_threadid_t server,
                       l4dm_dataspace_t *ds,
                       l4_addr_t start,
                       l4vfs_size_t length)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_mmap_munmap_call(&server, ds, start, length, 
                                 &_dice_corba_env);
}
