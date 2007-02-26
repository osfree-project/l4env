/**
 * \file   l4vfs/simple_file_server/server/resolve.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_SIMPLE_FILE_SERVER_SERVER_RESOLVE_H_
#define __L4VFS_SIMPLE_FILE_SERVER_SERVER_RESOLVE_H_

#include <l4/l4vfs/types.h>
#include <l4/sys/types.h>

#define IO_PATH_PARENT    ".."
#define IO_PATH_IDENTITY  '.'

object_id_t internal_resolve(object_id_t base,
                             const char * pathname);

char *  internal_rev_resolve(object_id_t dest,
                             object_id_t *parent);

extern volume_id_t my_volume_id;

#endif
