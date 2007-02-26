/**
 * \file   l4vfs/name_server/server/pathnames.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#define _GNU_SOURCE
#include <string.h>

#include "pathnames.h"
#include "resolve.h"

/* Split a pathname at the first path separator and return the first part.
 * Note: If using an absolute pathname you will get an empty string.
 */
char * get_first_path(const char * pathname)
{
    char * start;
    start = strchr(pathname, L4VFS_PATH_SEPARATOR);
    if (start) // found something
        return strndup(pathname, (int)(start - pathname));
    else       // no path separator found, so copy everything
        return strdup(pathname);
}

/* Split a pathname at the first path separator and return the last part.
 * The returned path must be freed externaly.
 */
char * get_remainder_path(const char * pathname)
{
    char * start;
    start = strchr(pathname, L4VFS_PATH_SEPARATOR);
    if (start) // found something
        return strdup(start + 1);
    else       // no path separator found, so return nothing
        return strdup("");
}

int is_absolute_path(const char * pathname)
{
    return (pathname[0] == L4VFS_PATH_SEPARATOR);
}

/* Check if this pathname points upwards (hi lars) in the tree
 */
int is_up_path(const char * pathname)
{
    if (strlen(pathname) == 2)
        if (strncmp(pathname, L4VFS_PATH_PARENT, 2) == 0)
            return true;
    return false;
}
