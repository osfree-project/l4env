/**
 * \file   l4vfs/name_server/server/resolve.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_NAME_SERVER_SERVER_RESOLVE_H_
#define __L4VFS_NAME_SERVER_SERVER_RESOLVE_H_

#include <l4/l4vfs/types.h>

object_id_t name_server_resolve(object_id_t base, const char * pathname);
char * name_server_rev_resolve(object_id_t dest, object_id_t * parent);
object_id_t local_resolve(object_id_t base, const char * dirname);
char * local_rev_resolve(object_id_t dest, object_id_t * parent);

#endif
