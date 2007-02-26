/**
 * \file   l4vfs/name_server/server/pathnames.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_NAME_SERVER_SERVER_PATHNAMES_H_
#define __L4VFS_NAME_SERVER_SERVER_PATHNAMES_H_

char * get_first_path(const char * pathname);
char * get_remainder_path(const char * pathname);
int is_absolute_path(const char * pathname);
int is_up_path(const char * pathname);

#endif
