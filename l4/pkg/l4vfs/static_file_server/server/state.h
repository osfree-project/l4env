/**
 * \file   l4vfs/static_file_server/server/state.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_STATIC_FILE_SERVER_SERVER_STATE_H_
#define __L4VFS_STATIC_FILE_SERVER_SERVER_STATE_H_

#include <l4/sys/types.h>

#define MAX_STATIC_FILES 32
#define STATIC_FILE_SERVER_VOLUME_ID 56
/* Object-id 0 is reserved for the root dir
 * 1 .. MAX_STATIC_FILES + 1 for the files
 */

typedef struct
{
    char * name;
    l4_uint8_t * data;
    int length;
} static_file_t;

extern static_file_t files[MAX_STATIC_FILES];

void state_init(void);

#endif
