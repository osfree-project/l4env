/**
 * \file   l4vfs/static_file_server/server/resolve.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_STATIC_FILE_SERVER_SERVER_RESOLVE_H_
#define __L4VFS_STATIC_FILE_SERVER_SERVER_RESOLVE_H_

#include <l4/l4vfs/types.h>
#include <l4/sys/types.h>

object_id_t internal_resolve(object_id_t base,
                             const char * pathname);

char *  internal_rev_resolve(object_id_t dest,
                             object_id_t *parent);

#endif
