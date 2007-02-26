/**
 * \file   l4vfs/lib/client/container_io.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/l4vfs/container_io.h>
#include <l4/l4vfs/container_io-client.h>
#include <l4/sys/types.h>

#include <stdlib.h>

int l4vfs_mkdir(l4_threadid_t server,
                const object_id_t *parent,
                const char* name,
                mode_t mode)
{
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    return l4vfs_container_io_mkdir_call(&server, parent, name, mode,
                                         &_dice_corba_env);
}
