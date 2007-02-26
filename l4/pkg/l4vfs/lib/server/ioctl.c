/**
 * \file   l4vfs/lib/server/ioctl.c
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
#include <common_io-server.h>

l4_int32_t
l4vfs_common_io_ioctl_component (CORBA_Object _dice_corba_obj,
                                 object_handle_t handle,
                                 int cmd,
                                 char **arg,
                                 l4vfs_size_t *count,
                                 CORBA_Server_Environment *_dice_corba_env)
    __attribute__((weak));

l4_int32_t
l4vfs_common_io_ioctl_component(CORBA_Object _dice_corba_obj,
                                object_handle_t fd,
                                int cmd,
                                char **arg,
                                l4vfs_size_t *count,
                                CORBA_Server_Environment *_dice_corba_env)
{
    return -ENOTTY;
}
