/**
 * \file   l4vfs/lib/server/fcntl.c
 * \brief  
 *
 * \date   09/13/2004
 * \author Carsten Weinhold  <cw183155@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <errno.h>
#include <common_io-server.h>
#include <l4/log/l4log.h>

l4_int32_t
l4vfs_common_io_fcntl_component(CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                int cmd,
                                long *arg,
                                CORBA_Server_Environment *_dice_corba_env)
    __attribute__((weak));

l4_int32_t
l4vfs_common_io_fcntl_component(CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                int cmd,
                                long *arg,
                                CORBA_Server_Environment *_dice_corba_env)
{
    LOG("weak fcntl; just returning 0");
    return 0;
}
