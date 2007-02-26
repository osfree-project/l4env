/**
 * \file   l4vfs/simple_file_server/server/resolve.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "resolve.h"
#include "state.h"

#include <l4/log/l4log.h>
#include <l4/simple_file_server/simple_file_server.h>

#include <string.h>

#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

object_id_t internal_resolve(object_id_t base, const char * pathname)
{
    int i;
    object_id_t ret;
    simple_file_t *s_file;

    LOGd(_DEBUG, "check root (%d.%d) '%s'",
         base.volume_id, base.object_id, pathname);
    // check root first
    if (strlen(pathname) == 1)
        if (pathname[0] == '/')
        {
            ret.volume_id = my_volume_id;
            ret.object_id = 0;
            return ret;
        }

    if (strcmp(pathname, ".") == 0) 
    {
         ret.volume_id = my_volume_id;
         ret.object_id = 0;
         return ret;

    }

    LOGd(_DEBUG, "check normal files");

    // now check the files
    for (i = 0; i < arraylist->size(files); i++)
    {
        s_file = (simple_file_t *) arraylist->get_elem(files,i);
        if (s_file->name != NULL)
        {
            LOGd(_DEBUG, "compare to name '%s'", s_file->name);
            if (strcmp(pathname, s_file->name) == 0)
            {
                ret.volume_id = my_volume_id;
                ret.object_id = i + 1;
                LOGd(_DEBUG, "got it '%s', (%d.%d)", s_file->name,
                     ret.volume_id, ret.object_id);
                return ret;
            }
        }
    }

    // nothing found
    ret.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
    ret.object_id = L4VFS_ILLEGAL_OBJECT_ID;
    return ret;
}
