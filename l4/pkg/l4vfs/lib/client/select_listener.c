/**
 * \file   l4vfs/lib/client/select_listener.c
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
#include <l4/l4vfs/select_listener.h>

int l4vfs_select_listener_send_notification(l4_threadid_t server,
                                             object_handle_t fd,
                                             int mode)
{
     CORBA_Environment _dice_corba_env = dice_default_environment;
     _dice_corba_env.malloc = (dice_malloc_func)malloc;
     _dice_corba_env.free = (dice_free_func)free;

     return l4vfs_select_listener_send_notification_call(&server,
                                                         fd,
                                                         mode,
                                                         &_dice_corba_env);
}

