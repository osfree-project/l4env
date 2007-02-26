/**
 * \file   l4vfs/lib/server/creat.c
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
#include <basic_io-server.h>

// provide some dummy impl.
object_handle_t
l4vfs_basic_io_creat_component(CORBA_Object _dice_corba_obj,
                               const object_id_t *parent,
                               const char* name,
                               l4_int32_t flags,
                               l4vfs_mode_t mode,
                               CORBA_Server_Environment *_dice_corba_env)
    __attribute__((weak));

object_handle_t
l4vfs_basic_io_creat_component(CORBA_Object _dice_corba_obj,
                               const object_id_t *parent,
                               const char* name,
                               l4_int32_t flags,
                               l4vfs_mode_t mode,
                               CORBA_Server_Environment *_dice_corba_env)
{
    return -EROFS;
}
