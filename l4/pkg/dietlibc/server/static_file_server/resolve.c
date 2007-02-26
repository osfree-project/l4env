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
    ret.volume_id = L4_IO_ILLEGAL_VOLUME_ID;
    ret.object_id = L4_IO_ILLEGAL_OBJECT_ID;
    return ret;
}

char *  internal_rev_resolve(object_id_t dest,
                             object_id_t *parent)
{
    // fix me: not impl. yet
    return NULL;
}

