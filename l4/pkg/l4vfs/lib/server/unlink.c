/**
 * \file   l4vfs/lib/server/unlink.c
 * \brief  
 *
 * \date   10/13/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <errno.h>
#include <basic_io-server.h>

// provide some dummy impl.
l4_int32_t
l4vfs_basic_io_unlink_component(CORBA_Object _dice_corba_obj,
                                const object_id_t *file,
                                CORBA_Server_Environment *_dice_corba_env)
    __attribute__((weak));

l4_int32_t
l4vfs_basic_io_unlink_component(CORBA_Object _dice_corba_obj,
                                const object_id_t *file,
                                CORBA_Server_Environment *_dice_corba_env)
{
    return -EROFS;
}
