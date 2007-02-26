/**
 * \file   l4vfs/include/extendable.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_EXTENDABLE_H_
#define __L4VFS_INCLUDE_EXTENDABLE_H_

#include <l4/l4vfs/types.h>
#include <l4/sys/types.h>
#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

int l4vfs_attach_namespace(l4_threadid_t server,
                           volume_id_t volume_id,
                           char * mounted_dir,
                           char * mount_dir);

int l4vfs_detach_namespace(l4_threadid_t server,
                           char * mount_dir);

EXTERN_C_END

#endif
