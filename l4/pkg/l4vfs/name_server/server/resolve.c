/**
 * \file   l4vfs/name_server/server/resolve.c
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
#include "dirs.h"
#include "pathnames.h"
#include "volumes.h"
#include "mount_table.h"

#include <l4/l4vfs/types.h>
#include <l4/l4vfs/basic_name_server.h>
#include <l4/log/l4log.h>

#include <string.h>
#include <stdlib.h>

extern int _DEBUG;

/* translate a base object-id and an relativ path from it into an
 * object_id
 */
object_id_t name_server_resolve(object_id_t base, const char * pathname)
{
    object_id_t ret;
    char * path_remainder;

    // special case for absolute pathnames
    if (is_absolute_path(pathname))
    {
        LOGd(_DEBUG, "absolute path: %s", pathname);
        path_remainder = get_remainder_path(pathname);
        LOGd(_DEBUG, "remainder: %s", path_remainder);
        base.volume_id = L4VFS_NAME_SERVER_VOLUME_ID;
        base.object_id = root.spec.dir.object_id;
    }
    else // normal case
    {
        path_remainder = strdup(pathname);
        LOGd(_DEBUG, "remainder: %s", path_remainder);
    }

    ret = base;

    // as long as we have something left in the path to resolve, go on
    while(strlen(path_remainder) > 0)
    {
        char * path, * temp;

        LOGd(_DEBUG, "1remainder: %s", path_remainder);

        path = get_first_path(path_remainder);
        LOGd(_DEBUG, "path: %s", path);
        // prepare mount point crossings
        if (is_up_path(path))
        { // this is the special case for stuff like ".."
            LOGd(_DEBUG, "up");
            if (is_mounted_point(ret))
            {
                LOGd(_DEBUG, ".");
                ret = translate_mounted_point(ret);
                LOGd(_DEBUG, "translate_mounted_point: (%d.%d)",
                     ret.volume_id, ret.object_id);
            }
        }
        else
        {
            LOGd(_DEBUG, "down");
            if (is_mount_point(ret))
            {
                LOGd(_DEBUG, ".");
                ret = translate_mount_point(ret);
                LOGd(_DEBUG, "translate_mount_point: (%d.%d)",
                     ret.volume_id, ret.object_id);
            }
        }

        // resolve one step
        if (is_own_volume(ret.volume_id))
        { // local
            LOGd(_DEBUG, "own volume");
            ret = local_resolve(ret, path);
            LOGd(_DEBUG, "local resolve: %d.%d", ret.volume_id, ret.object_id);
        }
        else
        { // remote
            l4_threadid_t remote;
            remote = server_for_volume(ret.volume_id);

            if (l4_is_invalid_id(remote))
            { // volume not known, name not resolvable
                ret.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
                ret.object_id = L4VFS_ILLEGAL_OBJECT_ID;
                break;
            }

            /* Note that we could optimize here:
             * If we know from the mount table that nothing is mounted below
             * we could send the whole remaining path to be resolved at once.
             */
            ret = l4vfs_resolve(remote, ret, path);
            LOGd(_DEBUG, "resolve: %d.%d", ret.volume_id, ret.object_id);
        }
        /* If we use the optimization above, nothing should remain in the
         * path.  So we could break out of the while loop.
         */
        // compute remainder of path
        temp = get_remainder_path(path_remainder);
        LOGd(_DEBUG, "temp: %s", temp);
        free(path_remainder);
        free(path);

        path_remainder = temp;

        if (ret.volume_id == L4VFS_ILLEGAL_VOLUME_ID)
        { // could not resolve, so free stuff and break out
            break;
        }
    };
    free(path_remainder);

    /* If we left the while loop, there should be a resolve
     * result in ret.
     *
     * only one thing left to do, translate possible mount_point
     */
    if (is_mount_point(ret))
    {
        LOGd(_DEBUG, ".");
        ret = translate_mount_point(ret);
        LOGd(_DEBUG, "translate_mount_point: (%d.%d)",
             ret.volume_id, ret.object_id);
    }

    return ret;
}

/* Reverse resolve an object id to a global name.
 * dest   is the object-id to be rev_resolved
 * parent is modified to contain the point whereto the translation has
 *        been concuted
 * returns the path between dest and the parent
 */
char * name_server_rev_resolve(object_id_t dest, object_id_t * parent)
{
    char * path, * ret, * temp;

    LOGd(_DEBUG, "rev_resolve(%d.%d, %d.%d)", dest.volume_id, dest.object_id,
        parent->volume_id, parent->object_id);
    path = strdup("");  // get some freeable memory
    // until we are at the base
    do
    {
        /* Translate dest according to mount_table. We can get such an
         * object_id as first argument
         */
        if (is_mounted_point(dest))
            dest = translate_mounted_point(dest);
        LOGd(_DEBUG, "path: '%s'", path);
        if (is_own_volume(dest.volume_id))
        { // from now on everything is local
            LOGd(_DEBUG, "local ...");
            parent->volume_id = L4VFS_NAME_SERVER_VOLUME_ID;
            parent->object_id = root.spec.dir.object_id;
            ret = local_rev_resolve(dest, parent);
            LOGd(_DEBUG, "ret: '%s'", ret);
        }
        else
        { // we have to look elsewhere
            l4_threadid_t remote;
            remote = server_for_volume(dest.volume_id);
            
            if (l4_is_invalid_id(remote))
            { // volume not known, name not resolvable
                parent->volume_id = L4VFS_ILLEGAL_VOLUME_ID;
                parent->object_id = L4VFS_ILLEGAL_OBJECT_ID;
                return NULL;
            }

            /* update parent, so that rev_resolve is bounded to
             * mounted point of volume
             */
            *parent = get_mounted_point(dest.volume_id);
            ret = l4vfs_rev_resolve(remote, dest, parent);
            dest.volume_id = parent->volume_id;
            dest.object_id = parent->object_id;
        }
        // we now have something in ret (maybe)
        if (ret == NULL) // something went wrong
        {
            free(path);
            return NULL;
        }

        // concatenate the two strings
        // we have "a/b", we got "x", we want "x/a/b"
        temp = malloc(strlen(ret) + strlen(path) + 2); // "\0" + "/"
        strcpy(temp, ret);
        if (strlen(path) > 0)
        {
            strcat(temp, "/");
            strcat(temp, path);
        }
        free(path);
        path = temp;
        free(ret);
    }
    while ( ! ((parent->volume_id == L4VFS_NAME_SERVER_VOLUME_ID) &&
               (parent->object_id == root.spec.dir.object_id)));

    LOGd(_DEBUG, "returning: '%s'", path);
    return path;
}

/* Resolves just one step locally.
 * That means dirname must not contain a dir_separator!
 */
object_id_t local_resolve(object_id_t base, const char * dirname)
{
    /* 1. get dir pointer from base object_id
     * 2. check if child exists for dirname
     * 3. return found object_id or illegal_object_id
     */

    obj_t * dir, * child_dir;
    object_id_t ret;

    LOGd(_DEBUG, "dirname = '%s', base = (%d.%d)",
         dirname, base.volume_id, base.object_id);
    // 1.
    dir = map_get_dir(base.object_id);
    if (dir == NULL)
    { // nothing found for name
        ret.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
        ret.object_id = L4VFS_ILLEGAL_OBJECT_ID;
        return ret;
    }
    LOGd(_DEBUG, "1");

    // 2.
    child_dir = dir_get_child(dir, dirname);
    LOGd(_DEBUG, "2");

    // 3.
    if (child_dir == NULL)
    { // nothing found for name
        ret.volume_id = L4VFS_ILLEGAL_VOLUME_ID;
        ret.object_id = L4VFS_ILLEGAL_OBJECT_ID;
        return ret;
    }
    LOGd(_DEBUG, "3");
    // something found for name
    if (child_dir->type == OBJ_DIR)
    {
        ret.volume_id = base.volume_id;
        ret.object_id = child_dir->spec.dir.object_id;
        return ret;
    }
    else
    {
        return child_dir->spec.file.oid;
    }
}

char * local_rev_resolve(object_id_t dest, object_id_t * parent)
{
    char * path, * ret, * temp;
    obj_t * actual;

    LOGd(_DEBUG, "local_rev_resolve(%d.%d, %d.%d)", dest.volume_id,
         dest.object_id, parent->volume_id, parent->object_id);
    // check some error conditions
    if (dest.volume_id != L4VFS_NAME_SERVER_VOLUME_ID)
        return NULL; // we got called accidentally

    actual = map_get_dir(dest.object_id);

    path = strdup("");
    // until we are at the root
    while (actual->spec.dir.object_id != root.spec.dir.object_id)
    {
        LOGd(_DEBUG, "path: '%s'", path);
        ret = actual->name;
        LOGd(_DEBUG, "actual->name: '%s'", ret);
        // we now have something in ret (maybe)
        if (ret == NULL) // something went wrong
        {
            free(path);
            return NULL;
        }

        // concatenate the two strings
        temp = malloc(strlen(ret) + strlen(path) + 2); // "\0" + "/"
        strcpy(temp, ret);
        if (strlen(path) > 0)
        {
            strcat(temp, "/");
            strcat(temp, path);
        }
        free(path);
        path = temp;
        actual = actual->parent;
    }

    // now add another '/' in front to create an absolute path
    temp = malloc(strlen(path) + 2); // "\0" + "/"
    strcpy(temp, "/");
    strcat(temp, path);
    free(path);
    LOGd(_DEBUG, "returning: '%s'", temp);

    return temp;
}
