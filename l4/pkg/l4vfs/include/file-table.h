/**
 * \file   l4vfs/include/file-table.h
 * \brief  File descriptor functions for filetable backend
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_FILE_TABLE_H_
#define __L4VFS_INCLUDE_FILE_TABLE_H_

#include <l4/sys/compiler.h>
#include <l4/l4vfs/types.h>

EXTERN_C_BEGIN

int         ft_get_next_free_entry(void);
void        ft_free_entry(int i);
void        ft_fill_entry(int i, file_desc_t fd_s);
int         ft_is_open(int i);
file_desc_t ft_get_entry(int i);
void        ft_init(void);

EXTERN_C_END

extern object_id_t cwd;     // current working directory
extern object_id_t c_root;  // current root (e.g. for chroot environments)

#endif
