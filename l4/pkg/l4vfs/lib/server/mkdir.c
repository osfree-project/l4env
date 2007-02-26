/**
 * \file   l4vfs/lib/server/mkdir.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <errno.h>
#include <container_io-server.h>

// provide some dummy impl.
int l4vfs_container_io_mkdir_component(CORBA_Object _dice_corba_obj,
                                       const object_id_t *parent,
                                       const char* name,
                                       l4vfs_mode_t mode,
                                       CORBA_Server_Environment *_dice_corba_env)
    __attribute__((weak));

int l4vfs_container_io_mkdir_component(CORBA_Object _dice_corba_obj,
                                       const object_id_t *parent,
                                       const char* name,
                                       l4vfs_mode_t mode,
                                       CORBA_Server_Environment *_dice_corba_env)
{
    return -EROFS;
}
