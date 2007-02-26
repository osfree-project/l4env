/**
 * \file   l4vfs/name_server/include/name_server.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_NAME_SERVER_INCLUDE_NAME_SERVER_H_
#define __L4VFS_NAME_SERVER_INCLUDE_NAME_SERVER_H_

#include <l4/sys/types.h>
#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

l4_threadid_t l4vfs_get_name_server_threadid(void);

EXTERN_C_END

#endif
