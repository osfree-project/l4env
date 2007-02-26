/*
 * \brief   grub loader specific plugin for file-I/O
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

#ifndef _IO_GRUBFS_H_
#define _IO_GRUBFS_H_

/*
 * fileops for using grub-loaded files via stdio-interface
 */
#include <sys/stat.h>

/*
 * working functions 
 */
int io_grubfs_open (const char *__name, int __mode, ...);
int io_grubfs_close (int __fd);

unsigned long io_grubfs_read (int __fd, void *__buf, unsigned long __n);
unsigned long io_grubfs_write (int __fd, void *__buf, unsigned long __n);

long io_grubfs_lseek (int __fd, long __offset, int __whence);

int io_grubfs_fstat (int __fd, struct stat *buf);

/*
 * not implemented
 */
int io_grubfs_ftruncate (int __fd, unsigned long __offset);

#endif
