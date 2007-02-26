/**
 * \file   l4vfs/include/select_notify.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_SELECT_NOTIFY_H_
#define __L4VFS_INCLUDE_SELECT_NOTIFY_H_

#include <l4/l4vfs/types.h>
#include <l4/l4vfs/select_notify-client.h>

void l4vfs_select_notify_request(l4_threadid_t server,
                                 object_handle_t fd,
                                 int mode,
                                 l4_threadid_t notif_tid);

void l4vfs_select_notify_clear(l4_threadid_t server,
                               object_handle_t fd,
                               int mode,
                               l4_threadid_t notif_tid);

#endif

