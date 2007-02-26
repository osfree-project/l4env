/**
 * \file   l4vfs/static_file_server/server/resolve.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "resolve.h"
#include "state.h"

#include <l4/log/l4log.h>

#include <string.h>

extern int _DEBUG;

object_id_t internal_resolve(object_id_t base, const char * pathname)
{
    int i;
    object_id_t ret;

    LOGd(_DEBUG, "check root (%d.%d) '%s'",
         base.volume_id, base.object_id, pathname);
    // check root first
    if (strlen(pathname) == 1)
        if (pathname[0] == '/')
        {
            ret.volume_id = STATIC_FILE_SERVER_VOLUME_ID;
            ret.object_id = 0;
            return ret;
        }

    // fix me: we should check '..', '.', '//', ...

    LOGd(_DEBUG, "check normal files");

    // now check the files
    for (i = 0; i < MAX_STATIC_FILES; i++)
    {
        if (files[i].name != NULL)
        {
            LOGd(_DEBUG, "compare to name '%s'", files[i].name);
            if (strcmp(pathname, files[i].name) == 0)
            {
                ret.volume_id = STATIC_FILE_SERVER_VOLUME_ID;
                ret.object_id = i + 1;
                return ret;
            }
        }
    }

    // nothing found
    ret.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
    ret.object_id = L4VFS_ILLEGAL_OBJECT_ID;
    return ret;
}
