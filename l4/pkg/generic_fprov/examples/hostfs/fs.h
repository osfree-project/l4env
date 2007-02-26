/*!
 * \file   generic_fprov/examples/hostfs/fs.h
 * \brief  file system interface
 *
 * \date   07/28/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __GENERIC_FPROV_EXAMPLES_HOSTFS_FS_H_
#define __GENERIC_FPROV_EXAMPLES_HOSTFS_FS_H_

#include <l4/sys/l4int.h>

typedef struct fs_file_t fs_file_t;

int fs_init(const char*diskname, const char*partname);
int fs_done(void);
struct fs_file_t* fs_open(const char*name, long long*size);
int fs_read(struct fs_file_t *file, long long off, char*addr, l4_size_t count);
int fs_close(struct fs_file_t*);

/* And the pendants running in their own thread */
int fs_do_init(const char*diskname, const char*partname);
int fs_do_done(void);
struct fs_file_t* fs_do_open(const char*name, long long*size);
int fs_do_read(struct fs_file_t *file, long long off, char*addr,
	       l4_size_t count);
int fs_do_close(struct fs_file_t*);

#endif
