/**
 * \file   l4vfs/include/container_io.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_CONTAINER_IO_H_
#define __L4VFS_INCLUDE_CONTAINER_IO_H_

#include <sys/types.h> /* for mode_t */
#include <l4/l4vfs/types.h>
#include <l4/l4vfs/container_io-client.h>

EXTERN_C_BEGIN

int l4vfs_mkdir(l4_threadid_t server,
                const object_id_t *parent,
                const char* name,
                mode_t mode);

EXTERN_C_END

#endif
