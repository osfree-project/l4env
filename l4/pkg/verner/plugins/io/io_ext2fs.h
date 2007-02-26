/*
 * \brief   Ext2fs specific plugin for file-I/O
 * \date    2004-03-17
 * \author  Carsten Rietzschel <cr7@os.inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2003  Carsten Rietzschel  <cr7@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef _IO_EXT2FS_H_
#define _IO_EXT2FS_H_

#include <sys/stat.h>

/* already implemented */
int io_ext2fs_open (const char *__name, int __mode, ...);
int io_ext2fs_close (int __fd);
unsigned long io_ext2fs_read (int __fd, void *__buf, unsigned long __n);
long io_ext2fs_lseek (int __fd, long __offset, int __whence);

/* not implemented */
unsigned long io_ext2fs_write (int __fd, void *__buf, unsigned long __n);
unsigned long io_ext2fs_fread (void *__buf, unsigned long __fact,
			       unsigned long __n, int __fd);
int io_ext2fs_ftruncate (int __fd, unsigned long __offset);
int io_ext2fs_fstat (int filedes, struct stat *buf);

#endif
