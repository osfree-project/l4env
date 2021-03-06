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

typedef struct chunk
{
    struct chunk        *next; //< queuing info
    l4dm_dataspace_t    *ds;   //< dataspace we keep a chunk in
    l4_umword_t         start; //< chunk start offset in file
    l4_size_t           size;  //< chunk size
    int                 refcnt; //< reference counter
} simple_file_chunk_t;

typedef struct
{
    char * name;                //< file name
    l4_uint8_t * data;          //< ptr to original file data
    int length;                 //< file size
    simple_file_chunk_t *chunks; //< mmap chunk list
} simple_file_t;

extern ARRAYLIST *files;
extern struct arraylist_services *arraylist;
extern struct arraylist_services services;

void state_init(void);

#endif
