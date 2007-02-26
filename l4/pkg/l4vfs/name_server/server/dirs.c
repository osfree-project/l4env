/**
 * \file   l4vfs/name_server/server/dirs.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "dirs.h"
#include "resolve.h"

#include <l4/l4vfs/types.h>
#include <l4/l4vfs/volume_ids.h>

#include <l4/log/l4log.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern int _DEBUG;

obj_t root;

// these are dynamicly changed to be converted to dirent structs
obj_t up;   // '..'
obj_t self; // '.'

static local_object_id_t object_id_counter = 0;
obj_t * object_id_map[NAME_SERVER_MAX_DIRS];

void init_dirs(int create_examples)
{
    LOGd(_DEBUG, "root ...");

    // setup the root node
    root.parent = &root;
    root.next = NULL;
    root.prev = NULL;
    root.type = OBJ_DIR;
    root.spec.dir.first_child = NULL;
    root.name = "";
    root.spec.dir.object_id = get_next_object_id();
    map_insert_dir(&root);

    // up and self are virtual nodes to create dirents
    up.name = "..";
    up.parent = NULL;
    up.next = NULL;
    up.prev = NULL;
    up.type = OBJ_DIR;
    up.spec.dir.first_child = NULL;
    up.spec.dir.object_id = L4VFS_ILLEGAL_OBJECT_ID;

    self.name = ".";
    self.parent = NULL;
    self.next = NULL;
    self.prev = NULL;
    self.type = OBJ_DIR;
    self.spec.dir.first_child = NULL;
    self.spec.dir.object_id = L4VFS_ILLEGAL_OBJECT_ID;

    // the following is an exemplary root name space
    if (create_examples)
    {
        LOGd(_DEBUG, "add child 'linux'");
        if (dir_add_child_dir(&root, "linux"))
            LOGd(_DEBUG, "something wrong with child 'linux'");
        LOGd(_DEBUG, "add child 'server'");
        if (dir_add_child_dir(&root, "server"))
            LOGd(_DEBUG, "something wrong with child 'server'");
        dir_add_child_dir(&root, "test1");
        dir_add_child_dir(&root, "test2");
        dir_add_child_dir(&root, "test3");
        dir_add_child_dir(root.spec.dir.first_child, "a");
        dir_add_child_dir(root.spec.dir.first_child, "b");
        dir_add_child_dir(root.spec.dir.first_child, "c");
        dir_add_child_dir(root.spec.dir.first_child, "a");
        {
            object_id_t oid;
            oid.volume_id = STATIC_FILE_SERVER_VOLUME_ID;
            oid.object_id = 1;
            dir_add_child_file(&root, "file1", oid);
        }
    }
}

/* Create and add a new directory as child of parent.
 */
int dir_add_child_dir(obj_t * parent, const char * name)
{
    obj_t * child;
    int ret;

    LOGd(_DEBUG, "add child ...");
    // check for strange errors
    if (! dir_is_dir(parent))
    {
        LOGd(_DEBUG, "dir_add_child_dir() called with non-dir!");
        return 1000;
    }

    // check name first
    if ((ret = check_dir_name(parent, name)))
    {
        // found illegal or duplicate name
        return ret;
    }

    LOGd(_DEBUG, "... name check passed ...");

    child = malloc(sizeof(obj_t));
    child->type = OBJ_DIR;
    child->spec.dir.first_child = NULL;
    child->spec.dir.object_id = get_next_object_id();
    child->prev = NULL;
    child->parent = parent;
    child->next = parent->spec.dir.first_child;
    LOGd(_DEBUG, "1");
    child->name = strdup(name);
    LOGd(_DEBUG, "2");
    parent->spec.dir.first_child = child;
    map_insert_dir(child);

    LOGd(_DEBUG, "add child !");

    return 0;  // everything is ok
}

/* Create and add a new file as child of parent.
 * As these have remote object_ids we don't need to keep them in the map.
 */
int dir_add_child_file(obj_t * parent, const char * name, object_id_t oid)
{
    obj_t * child;
    int ret;

    LOGd(_DEBUG, "add child ...");
    // check for strange errors
    if (! dir_is_dir(parent))
    {
        LOGd(_DEBUG, "dir_add_child_file() called with non-dir!");
        return 1000;
    }

    // check name first
    if ((ret = check_dir_name(parent, name)))
    {
        // found illegal or duplicate name
        return ret;
    }

    LOGd(_DEBUG, "... name check passed ...");

    child = malloc(sizeof(obj_t));
    child->type = OBJ_FILE;
    child->spec.file.oid = oid;
    child->prev = NULL;
    child->parent = parent;
    child->next = parent->spec.dir.first_child;
    LOGd(_DEBUG, "1");
    child->name = strdup(name);
    LOGd(_DEBUG, "2");
    parent->spec.dir.first_child = child;

    LOGd(_DEBUG, "add child !");

    return 0;  // everything is ok
}
/* Find a named child for a given dir.
 */
obj_t * dir_get_child(obj_t * dir, const char * name)
{
    obj_t * actual;

    // check for strange errors
    if (! dir_is_dir(dir))
    {
        LOGd(_DEBUG, "dir_get_child() called with non-dir!");
        return NULL;
    }

    // check some special cases first
    if (strlen(name) == 0) // '/xyz/abc//123/er
        return dir;
    if (strlen(name) == 1) // '/xyz/abc/./123/er
        if (name[0] == L4VFS_PATH_IDENTITY)
            return dir;
    if (strlen(name) == 2) // '/xyz/abc/../123/er
        if (strncmp(name, L4VFS_PATH_PARENT, 2) == 0)
            return dir->parent;

    // now the normal cases
    actual = dir->spec.dir.first_child;
    while (actual)
    {
        if (strcmp(name, actual->name) == 0)
            return actual;
        actual = actual->next;
    }

    return NULL; // nothing found
}

/* Create a new unique local object_id.
 */
local_object_id_t get_next_object_id()
{
    return object_id_counter++;
}

/* checks for duplicate entries in one dir
 * Can also check for illegal characters in name
 */
int check_dir_name(obj_t * parent, const char * name)
{
    char * pos;
    obj_t * actual;

    // check for strange errors
    if (! dir_is_dir(parent))
    {
        LOGd(_DEBUG, "check_dir_name() called with non-dir!");
        return 1000;
    }

    // check for illegal characters
    pos = strpbrk(name, L4VFS_ILLEGAL_OBJECT_NAME_CHARS);
    if (pos != NULL)
        return 1; // found an illegal character

    // check for reserved names
    if (strlen(name) == 1)
    {
        /*
        // this should be found by above check
        if (name[0] == L4VFS_PATH_SEPARATOR)
            return 2; // "/"
        */
        if (name[0] == L4VFS_PATH_IDENTITY)
            return 2; // "."
    }
    if (strlen(name) == 2)
    {
        if (strcmp(name, L4VFS_PATH_PARENT) == 0)
            return 2; // ".."
    }

    // check for duplicates
    actual = parent->spec.dir.first_child;
    while (actual)
    {
        if (strcmp(name, actual->name) == 0)
            return 3; // found the wanted name
        actual = actual->next;
    }

    return 0; // everything is ok
}

/* More or less debug stuff,
 * recursively print the local tree.
 */
void print_tree(int indent, obj_t * dir)
{
    int i;
    obj_t * actual;
    for (i = 0; i < indent; i++)
        printf("    ");
    if (! dir_is_dir(dir))
    {
        printf("%s (%d.%d)\n", dir->name, dir->spec.file.oid.volume_id,
               dir->spec.file.oid.object_id);
        return; // just a file, return
    }
    else
    {
        printf("%s (%d)/\n", dir->name, dir->spec.dir.object_id);
        actual = dir->spec.dir.first_child;
        while (actual)
        {
            print_tree(indent + 1, actual);
            actual = actual->next;
        }
    }
}

/* Translates a local obejct_id to a pointer to the dir
 */
obj_t * map_get_dir(local_object_id_t object_id)
{
    return object_id_map[object_id];
}

/* Inserts a pointer to a dir for the given id
 */
void map_insert_dir(obj_t * dir)
{
    object_id_map[dir->spec.dir.object_id] = dir;
}

/* Fill a dirent struct based on the data given with the obj_t
 */
void dir_to_dirent(obj_t * dirp, l4vfs_dirent_t * entry)
{
    entry->d_ino = dirp->spec.dir.object_id;
    entry->d_off = -1; // this is not really defined, linux kernel
                       // returnes slightly wrong number, at least for
                       // some FSs
    strncpy(entry->d_name, dirp->name, L4VFS_MAX_NAME);
    entry->d_name[L4VFS_MAX_NAME] = '\0';

    entry->d_reclen = dir_dirent_size(dirp);
}

/* Compute the size of a resulting dirent struct, care for padding and stuff
 */
int dir_dirent_size(obj_t * dirp)
{
    int len;
    l4vfs_dirent_t * temp;
    // + 1 for the '\0'
    len = sizeof(temp->d_ino) + sizeof(temp->d_off) +
          sizeof(temp->d_reclen) + strlen(dirp->name) + 1;
    // round up to align to word size
    len = (len + sizeof(int) - 1);
    len = (len / sizeof(int)) * sizeof(int);
    return len;
}

/* Fill a client buffer starting from direntry index with up to length
 * bytes.
 * return written bytes (in 'int * length') and new seek pos
 */
int dir_fill_dirents(obj_t * dirp, int index,
                     l4vfs_dirent_t * entry, int * length)
{
    int len, count = 0, i;
    obj_t * actual;

    // '.'
    if (index == 0)
    {
        len = dir_dirent_size(&self);
        if (count + len <= *length)
        {
            self.spec.dir.object_id = dirp->spec.dir.object_id;
            dir_to_dirent(&self, entry);
            entry = (l4vfs_dirent_t *)(((char *)entry) + entry->d_reclen);
            index++;
            count += len;
        }
        else
        {
            if (count == 0)
                * length = -1;
            else
                * length = count;
            return 0;
        }
    }

    // '..'
    if (index == 1)
    {
        len = dir_dirent_size(&up);
        if (count + len <= *length)
        {
            up.spec.dir.object_id = dirp->parent->spec.dir.object_id;
            dir_to_dirent(&up, entry);
            entry = (l4vfs_dirent_t *)(((char *)entry) + entry->d_reclen);
            index++;
            count += len;
        }
        else
        {
            if (count == 0)
                * length = -1;
            else
                * length = count;
            return 1;
        }
    }

    // search next relevant entry
    actual = dirp->spec.dir.first_child;
    for (i = 0; (actual != NULL) && (i + 2 < index);
         i++, actual = actual->next)
        ;
    if (actual == NULL)
    {
        * length = count;
        return i + 2;
    }

    // now the normal dirs
    while (actual)
    {
        len = dir_dirent_size(actual);
        if (count + len <= *length)
        {
            dir_to_dirent(actual, entry);
            entry = (l4vfs_dirent_t *)(((char *)entry) + entry->d_reclen);
            index++;
            count += len;
        }
        else
        {
            // buffer full
            if (count == 0)
                * length = -1;
            else
                * length = count;
            return index;
        }
        actual = actual->next;
    }
    // EOF
    * length = count;
    return index;
}

/* Check if a obj is a dir.
 * Returns true if it is, false otherwise.
 */
int dir_is_dir(obj_t * obj)
{
    return obj->type == OBJ_DIR;
}
