/**
 * \file   l4vfs/lib/server/getpeername.c
 * \brief  
 *
 * \date   2006-10-18
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <errno.h>
#include <net_io-server.h>
#include <l4/log/l4log.h>

l4_int32_t
l4vfs_net_io_getpeername_component (CORBA_Object _dice_corba_obj,
                                    object_handle_t handle,
                                    char addr[L4VFS_SOCKET_MAX_ADDRLEN],
                                    int *addrlen,
                                    int *actual_len,
                                    CORBA_Server_Environment *_dice_corba_env)
    __attribute__((weak));

l4_int32_t
l4vfs_net_io_getpeername_component (CORBA_Object _dice_corba_obj,
                                    object_handle_t handle,
                                    char addr[L4VFS_SOCKET_MAX_ADDRLEN],
                                    int *addrlen,
                                    int *actual_len,
                                    CORBA_Server_Environment *_dice_corba_env)
{
    LOG("l4vfs_net_io_getpeername_component is not implemented!");
    return -ENOTSOCK;
}
