/*!
 * \file   libfile.h
 * \brief  Header file
 *
 * \date   2008-02-27
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2008 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __LIBFILE_H_
#define __LIBFILE_H_

#include <stddef.h>

int open64(const char *name, int flags, int mode);
int open(const char *name, int flags, int mode);
size_t read(int fd, void *buf, size_t count);
unsigned int lseek(int fd, unsigned int offset, int whence);
int close(int);

#endif /* __LIBFILE_H__ */
