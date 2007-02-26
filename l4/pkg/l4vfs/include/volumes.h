/**
 * \file   l4vfs/include/volumes.h
 * \brief  Volume management functions and types for filetable backend
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_VOLUMES_H_
#define __L4VFS_INCLUDE_VOLUMES_H_

#include <l4/sys/compiler.h>
#include <l4/l4vfs/types.h>

#define NAME_SERVER_MAX_VOLUMES 32

typedef struct
{
    volume_id_t   volume_id;
    l4_threadid_t server_id;
} volume_entry_t;

extern volume_entry_t volume_entries[NAME_SERVER_MAX_VOLUMES];

EXTERN_C_BEGIN

void init_volume_entries(void);
l4_threadid_t server_for_volume(volume_id_t volume_id);
int index_for_volume(volume_id_t volume_id);
int insert_volume_server(volume_id_t volume_id, l4_threadid_t server_id);
int remove_volume_server(volume_id_t volume_id, l4_threadid_t server_id);
int first_empty_volume_entry(void);
int vol_resolve_thread_for_volume_id(volume_id_t v_id, l4_threadid_t * srv);

EXTERN_C_END

#endif
