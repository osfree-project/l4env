/**
 * \file   l4vfs/lib/tree_helper/paths.c
 * \brief  
 *
 * \date   11/05/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/l4vfs/tree_helper.h>
#include <l4/l4vfs/types.h>

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>

int l4vfs_th_is_absolute_path(const char * path)
{
    return (path[0] == L4VFS_PATH_SEPARATOR);
}

char * l4vfs_th_get_first_path(const char * name)
{
    char * start;
    start = strchr(name, L4VFS_PATH_SEPARATOR);
    if (start) // found something
        return strndup(name, (int)(start - name));
    else       // no path separator found, so copy everything
        return strdup(name);
}

/* Split a pathname at the first path separator and return the last part.
 * The returned path must be freed externaly.
 */
char * l4vfs_th_get_remainder_path(const char * name)
{
    char * start;
    start = strchr(name, L4VFS_PATH_SEPARATOR);
    if (start) // found something
        return strdup(start + 1);
    else       // no path separator found, so return nothing
        return strdup("");
}
