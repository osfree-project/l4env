/**
 * \file   l4vfs/lib/server/access.c
 * \brief  
 *
 * \date   10/27/2004
 * \author Carsten Weinhold  <cw183155@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <basic_io-server.h>
#include <l4/log/l4log.h>

l4_int32_t
l4vfs_basic_io_access_component(CORBA_Object _dice_corba_obj,
                                const object_id_t *file,
                                l4_int32_t mode,
                                CORBA_Server_Environment *_dice_corba_env)
    __attribute__((weak));

l4_int32_t
l4vfs_basic_io_access_component(CORBA_Object _dice_corba_obj,
                                const object_id_t *file,
                                l4_int32_t mode,
                                CORBA_Server_Environment *_dice_corba_env)
{
    LOG("weak access; just returning 0");
    return 0;
}
