/**
 * \file   l4vfs/simple_file_server/server/dirs.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_SIMPLE_FILE_SERVER_SERVER_DIRS_H_
#define __L4VFS_SIMPLE_FILE_SERVER_SERVER_DIRS_H_

#include <l4/l4vfs/types.h>

#include <dirent.h>

void init_dirs(void);
int fill_dirents(int index, l4vfs_dirent_t * entry, int * length);
void convert_to_dirent(int object_id, char *fname, l4vfs_dirent_t * entry);
int dirent_size(char *fname);

#endif
