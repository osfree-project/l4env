/**
 * \file   l4vfs/include/select_listener.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_SELECT_LISTENER_H_
#define __L4VFS_INCLUDE_SELECT_LISTENER_H_

#include <l4/l4vfs/types.h>
#include <l4/l4vfs/select_listener-client.h>

EXTERN_C_BEGIN

int l4vfs_select_listener_send_notification(l4_threadid_t server,
                                            object_handle_t fd,
                                            int mode);

int l4vfs_select_listener_start_listening(l4_threadid_t server,
                               object_handle_t *fd,
                               int *mode);

void l4vfs_select_listener_init_listener(l4_threadid_t server,
                                         l4_uint32_t timout);

EXTERN_C_END

#endif

