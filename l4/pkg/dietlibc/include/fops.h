/*!
 * \file   dietlibc/include/fops.h
 * \brief  
 *
 * \date   08/16/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __DIETLIBC_INCLUDE_FOPS_H_
#define __DIETLIBC_INCLUDE_FOPS_H_
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#define L4DIET_VFS_MAXFILES 64
typedef struct l4diet_vfs_file{
    void    *priv;

    int     (*close)   (struct l4diet_vfs_file*);
    int     (*read)    (struct l4diet_vfs_file*, void*buf, size_t count);
    int     (*write)   (struct l4diet_vfs_file*, const void*buf, size_t count);
    off_t   (*lseek)   (struct l4diet_vfs_file*, off_t offset, int whence);
    int     (*fstat)   (struct l4diet_vfs_file*, struct stat *buf);
    void*   (*mmap)    (void *start, size_t length, int prot, int flags,
			struct l4diet_vfs_file*, off_t offset);
} l4diet_vfs_file;

extern l4diet_vfs_file l4diet_vfs_file1, l4diet_vfs_file2;

extern l4diet_vfs_file* l4diet_vfs_files[L4DIET_VFS_MAXFILES];
extern l4diet_vfs_file* l4diet_vfs_alloc_nr(int nr);
#endif
