/**
 * \file   l4vfs/name_server/server/volumes.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_NAME_SERVER_SERVER_VOLUMES_H_
#define __L4VFS_NAME_SERVER_SERVER_VOLUMES_H_

#include <l4/l4vfs/types.h>

#define NAME_SERVER_MAX_VOLUMES 32

typedef struct
{
    object_id_t   root_id;
    l4_threadid_t server_id;
} volume_entry_t;

extern volume_entry_t volume_entries[NAME_SERVER_MAX_VOLUMES];

void init_volume_entries(void);
l4_threadid_t server_for_volume(volume_id_t volume_id);
local_object_id_t root_for_volume(volume_id_t volume_id);
int index_for_volume(volume_id_t volume_id);
int insert_volume_server(object_id_t root_id, l4_threadid_t server_id);
int remove_volume_server(volume_id_t volume_id, l4_threadid_t server_id);
int first_empty_volume_entry(void);
int is_own_volume(volume_id_t volume_id);

#endif
