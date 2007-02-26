#include "dirs.h"
#include "resolve.h"

#include <l4/dietlibc/io-types.h>

#include <l4/log/l4log.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

extern int _DEBUG;

dir_t root;
dir_t up;   // '..'
dir_t self; // '.'

static local_object_id_t object_id_counter = 0;
dir_t * object_id_map[NAME_SERVER_MAX_DIRS];

void init_dirs()
{
    LOGd(_DEBUG, "root ...");
    root.parent = &root;
    root.next = NULL;
    root.prev = NULL;
    root.first_child = NULL;
    root.name = "";
    root.object_id = get_next_object_id();
    map_insert_dir(&root);

    up.name = "..";
    up.parent = NULL;
    up.next = NULL;
    up.prev = NULL;
    up.first_child = NULL;
    up.object_id = L4_IO_ILLEGAL_OBJECT_ID;

    self.name = ".";
    self.parent = NULL;
    self.next = NULL;
    self.prev = NULL;
    self.first_child = NULL;
    self.object_id = L4_IO_ILLEGAL_OBJECT_ID;


    LOGd(_DEBUG, "!");

    LOGd(_DEBUG, "add child 'linux'");
    if (dir_add_child(&root, "linux"))
        LOGd(_DEBUG, "something wrng with child 'linux'");
    LOGd(_DEBUG, "add child 'server'");
    if (dir_add_child(&root, "server"))
        LOGd(_DEBUG, "something wrng with child 'server'");
    dir_add_child(&root, "test1");
    dir_add_child(&root, "test2");
    dir_add_child(&root, "test3");
    dir_add_child(root.first_child, "a");
    dir_add_child(root.first_child, "b");
    dir_add_child(root.first_child, "c");
    dir_add_child(root.first_child, "a");
    LOGd(_DEBUG, "!");
}

int dir_add_child(dir_t * parent, const char * name)
{
    dir_t * child;

    LOGd(_DEBUG, "add child ...");
    // check name first
    if (check_dir_name(parent, name))
    {
        // found illegal or duplicate name
        return 1;
    }

    LOGd(_DEBUG, "... name check passed ...");

    child = malloc(sizeof(dir_t));
    child->first_child = NULL;
    child->prev = NULL;
    child->parent = parent;
    child->next = parent->first_child;
    child->object_id = get_next_object_id();
    LOGd(_DEBUG, "1");
    child->name = strdup(name);
    LOGd(_DEBUG, "2");
    parent->first_child = child;
    map_insert_dir(child);

    LOGd(_DEBUG, "add child !");
    
    return 0;  // everything is ok
}

/* Find a named child for a given dir.
 */
dir_t * dir_get_child(dir_t * dir, const char * name)
{
    dir_t * actual;

    // check some special cases first
    if (strlen(name) == 0) // '/xyz/abc//123/er
        return dir;
    if (strlen(name) == 1) // '/xyz/abc/./123/er
        if (name[0] == IO_PATH_IDENTITY)
            return dir;
    if (strlen(name) == 2) // '/xyz/abc/../123/er
        if (strncmp(name, IO_PATH_PARENT, 2) == 0)
            return dir->parent;

    // now the normal cases
    actual = dir->first_child;
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
int check_dir_name(dir_t * parent, const char * name)
{
    char * pos;
    dir_t * actual;

    // check for illegal characters
    pos = strpbrk(name, L4_IO_ILLEGAL_OBJECT_NAME_CHARS);
    if (pos != NULL)
        return 1; // found an illegal character

    // check for reserved names
    if (strlen(name) == 1)
    {        
        /*
        // this should be found by above check
        if (name[0] == IO_PATH_SEPARATOR)
            return 2; // "/"
        */
        if (name[0] == IO_PATH_IDENTITY)
            return 2; // "."
    }
    if (strlen(name) == 2)
    {        
        if (strcmp(name, IO_PATH_PARENT) == 0)
            return 2; // ".."
    }

    // check for duplicates
    actual = parent->first_child;
    while (actual)
    {
        if (strcmp(name, actual->name) == 0)
            return 3; // found the wanted name
        actual = actual->next;
    }

    return 0; // everything is ok
}

// more or less debug stuff
// recursively print the local tree
void print_tree(int indent, dir_t * dir)
{
    int i;
    dir_t * actual;
    for (i = 0; i < indent; i++)
        printf("    ");
    printf("%s (%d)/\n", dir->name, dir->object_id);
    actual = dir->first_child;
    while (actual)
    {
        print_tree(indent + 1, actual);
        actual = actual->next;
    }
}

/* Translates a local obejct_id to a pointer to the dir
 */
dir_t * map_get_dir(local_object_id_t object_id)
{
    return object_id_map[object_id];
}

/* Inserts a pointer to a dir for the given id
 */
void map_insert_dir(dir_t * dir)
{
    object_id_map[dir->object_id] = dir;
}

void dir_to_dirent(dir_t * dirp, struct dirent * entry)
{
    entry->d_ino = dirp->object_id;
    entry->d_off = -1; // this is not really defined, linux kernel
                       // returnes slightly wrong number, at least for
                       // some FSs
    strncpy(entry->d_name, dirp->name, MAX_NAME);
    entry->d_name[MAX_NAME + 1] = '\0';

    entry->d_reclen = dir_dirent_size(dirp);
}

int dir_dirent_size(dir_t * dirp)
{
    int len;
    struct dirent * temp;
    // + 1 for the '\0'
    len = sizeof(temp->d_ino) + sizeof(temp->d_off) +
          sizeof(temp->d_reclen) + strlen(dirp->name) + 1;
    // round up to align to word size
    len = (len + sizeof(int) - 1);
    len = len / sizeof(int);
    len = len * sizeof(int);
    return len;
}

// compute size for complete buffer for a dir
int dir_childs_dirent_size(dir_t * dirp)
{
    dir_t * actual;
    int len = 0;

    actual = dirp->first_child;
    while (actual)
    {
        len += dir_dirent_size(actual);
        actual = actual->next;
    }
    len += dir_dirent_size(&up);
    len += dir_dirent_size(&self);

    return len;
}

// allocate and fill a complete dir buffer
void dir_childs_to_dirents(dir_t * dirp, struct dirent ** entry, int * length)
{
    int len;
    struct dirent * p;
    dir_t * actual;

    len = dir_childs_dirent_size(dirp);
    *length = len;
    *entry = malloc(len);
    p = (struct dirent *)*entry;

    // special cases first
    // '.'
    self.object_id = dirp->object_id;
    dir_to_dirent(&self, p);
    p = (struct dirent *)(((char *)p) + p->d_reclen);
    // '..'
    up.object_id = dirp->parent->object_id;
    dir_to_dirent(&up, p);
    p = (struct dirent *)(((char *)p) + p->d_reclen);
    actual = dirp->first_child;
    while (actual)
    {
        dir_to_dirent(actual, p);
        p = (struct dirent *)(((char *)p) + p->d_reclen);
        actual = actual->next;
    }
}
