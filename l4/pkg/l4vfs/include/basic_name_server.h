/**
 * \file   l4vfs/include/basic_name_server.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_BASIC_NAME_SERVER_H_
#define __L4VFS_INCLUDE_BASIC_NAME_SERVER_H_

#include <l4/l4vfs/types.h>
#include <l4/sys/types.h>

EXTERN_C_BEGIN

object_id_t   l4vfs_resolve(l4_threadid_t server,
                            object_id_t base,
                            const char * pathname);

char *        l4vfs_rev_resolve(l4_threadid_t server,
                                object_id_t dest,
                                object_id_t *parent);

l4_threadid_t l4vfs_thread_for_volume(l4_threadid_t server,
                                      volume_id_t volume_id);

EXTERN_C_END

#endif
