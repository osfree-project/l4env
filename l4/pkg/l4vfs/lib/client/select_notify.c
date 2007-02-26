/**
 * \file   l4vfs/lib/client/select_notify.c
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
#include <l4/l4vfs/select_notify.h>

void l4vfs_select_notify_request(l4_threadid_t server,
                                 object_handle_t fd,
                                 int mode,
                                 l4_threadid_t notif_tid)
{
     CORBA_Environment _dice_corba_env = dice_default_environment;
     _dice_corba_env.malloc = (dice_malloc_func)malloc;
     _dice_corba_env.free = (dice_free_func)free;

     l4vfs_select_notify_request_send(&server,
                                      fd,
                                      mode,
                                      &notif_tid,
                                      &_dice_corba_env);
}

void l4vfs_select_notify_clear(l4_threadid_t server,
                               object_handle_t fd,
                               int mode,
                               l4_threadid_t notif_tid)
{
     CORBA_Environment _dice_corba_env = dice_default_environment;
     _dice_corba_env.malloc = (dice_malloc_func)malloc;
     _dice_corba_env.free = (dice_free_func)free;

     l4vfs_select_notify_clear_send(&server,
                                    fd,
                                    mode,
                                    &notif_tid,
                                    &_dice_corba_env);
}

