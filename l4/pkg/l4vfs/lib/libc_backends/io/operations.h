/**
 * \file   l4vfs/lib/libc_backends/io/operations.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_LIB_BACKENDS_IO_OPERATIONS_H_
#define __L4VFS_LIB_BACKENDS_IO_OPERATIONS_H_

// these should not be necesarry, as they are defined in libc header
// files
/*
int open(const char *pathname, int flags, mode_t mode);
int close(int fd);
int read(int fd, void *buf, size_t count);
*/

void init_io(void);

#endif
