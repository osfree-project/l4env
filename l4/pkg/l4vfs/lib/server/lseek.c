/**
 * \file   l4vfs/lib/server/lseek.c
 * \brief  
 *
 * \date   10/25/2004
 * \author Martin Pohlack  <mp26@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <errno.h>
#include <common_io-server.h>
#include <l4/log/l4log.h>

l4vfs_off_t
l4vfs_basic_io_lseek_component(CORBA_Object _dice_corba_obj,
                               object_handle_t fd,
                               l4vfs_off_t offset,
                               l4_int32_t whence,
                               CORBA_Server_Environment *_dice_corba_env)
    __attribute__((weak));

l4vfs_off_t
l4vfs_basic_io_lseek_component(CORBA_Object _dice_corba_obj,
                               object_handle_t fd,
                               l4vfs_off_t offset,
                               l4_int32_t whence,
                               CORBA_Server_Environment *_dice_corba_env)
{
    LOG("weak lseek; just returning 0");
    return 0;
}
