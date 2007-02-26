/**
 * \file   l4vfs/include/name_space_provider.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_NAME_SPACE_PROVIDER_H_
#define __L4VFS_INCLUDE_NAME_SPACE_PROVIDER_H_

#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

int l4vfs_register_volume(l4_threadid_t dest, l4_threadid_t server,
                          object_id_t root_id);

int l4vfs_unregister_volume(l4_threadid_t dest, l4_threadid_t server,
                            volume_id_t volume_id);

EXTERN_C_END

#endif
