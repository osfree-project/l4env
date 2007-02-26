/**
 * \file   l4vfs/include/mmap_io.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_MMAP_IO_H_
#define __L4VFS_INCLUDE_MMAP_IO_H_

#include <l4/sys/compiler.h>
#include <l4/l4vfs/mmap-client.h>

EXTERN_C_BEGIN

// this one is postfixed with '1' due to a dice-created name clash
l4_int32_t l4vfs_mmap1(l4_threadid_t server,
                       l4dm_dataspace_t *ds,
                       l4vfs_size_t length,
                       l4_int32_t prot,
                       l4_int32_t flags,
                       object_handle_t fd,
                       l4vfs_off_t offset);

l4_int32_t l4vfs_msync(l4_threadid_t server,
                       l4dm_dataspace_t *ds,
                       l4_addr_t start,
                       l4vfs_size_t length,
                       l4_int32_t flags);

l4_int32_t l4vfs_munmap(l4_threadid_t server,
                       l4dm_dataspace_t *ds,
                       l4_addr_t start,
                       l4vfs_size_t length);

EXTERN_C_END

#endif
