/**
 * \file   l4vfs/simple_file_server/server/state.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_SIMPLE_FILE_SERVER_SERVER_STATE_H_
#define __L4VFS_SIMPLE_FILE_SERVER_SERVER_STATE_H_

#include <l4/sys/types.h>
#include <l4/dm_mem/dm_mem.h>
#include "arraylist.h"

/* Object-id 0 is reserved for the root dir
 * 1 .. MAX_STATIC_FILES for the files
 */

typedef struct
{
    char * name;
    l4_uint8_t * data;
    int length;
    l4dm_dataspace_t *ds;
    int ds_ref_count;
} simple_file_t;

extern ARRAYLIST *files;
extern struct arraylist_services *arraylist;
extern struct arraylist_services services;

void state_init(void);

#endif
