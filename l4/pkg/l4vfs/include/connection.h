/**
 * \file   l4vfs/include/connection.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_CONNECTION_H_
#define __L4VFS_INCLUDE_CONNECTION_H_

#include <l4/l4vfs/types.h>
#include <l4/l4vfs/connection-client.h>

EXTERN_C_BEGIN

l4_threadid_t	l4vfs_init_connection(l4_threadid_t server);

void		l4vfs_close_connection(l4_threadid_t server, l4_threadid_t connection);

EXTERN_C_END

#endif

