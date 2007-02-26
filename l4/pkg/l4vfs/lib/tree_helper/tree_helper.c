/**
 * \file   l4vfs/lib/tree_helper/tree_helper.c
 * \brief  
 *
 * \date   10/25/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/l4vfs/tree_helper.h>
#include <l4/l4vfs/types.h>

#include <l4/log/l4log.h>

#include <stdlib.h>
#include <string.h>


#define TREE_HELPER_MAX_OBJECTS 1024


static local_object_id_t id_counter = 0;
static l4vfs_th_node_t * object_table[TREE_HELPER_MAX_OBJECTS];


static local_object_id_t _get_new_id(void)
{
    return id_counter++;
}

l4vfs_th_node_t * l4vfs_th_node_for_id(local_object_id_t id)
{
    int i;

    for (i = 0; i < TREE_HELPER_MAX_OBJECTS; i++)
    {
        if (object_table[i] != NULL && object_table[i]->id == id)
            return object_table[i];
    }
    return NULL;
}

l4vfs_th_node_t * l4vfs_th_child_for_name(l4vfs_th_node_t * parent,
                                          const char * name)
{
    l4vfs_th_node_t * child;

    child = parent->first_child;
    while (child)
    {
        if (strcmp(child->name, name) == 0)
            return child;
        child = child->next;
    }
    return NULL;
}

l4vfs_th_node_t * l4vfs_th_new_node(const char * name, int type,
                                    l4vfs_th_node_t * parent,
                                    void * data)
{
    l4vfs_th_node_t * node, * temp;
    int i;

    node = malloc(sizeof(l4vfs_th_node_t));
    if (node == NULL)
        return NULL;

    node->name = strdup(name);
    if (node->name == NULL)
    {
        free(node);
        return NULL;
    }
    node->data        = data;
    node->type        = type;
    node->id          = _get_new_id();
    node->usage_count = 0;
    node->first_child = NULL;

    // insert into table
    for (i = 0; i < TREE_HELPER_MAX_OBJECTS; i++)
    {
        if (object_table[i] == NULL)
        {
            object_table[i] = node;
            break;
        }
    }
    if (i == TREE_HELPER_MAX_OBJECTS)  // no space left in table
    {
        free(node->name);
        free(node);
    }

    // insert into tree
    node->parent = parent;
    if (parent != NULL)
    {
        temp = parent->first_child;
        node->next = temp;
        parent->first_child = node;
    }
    else
    {
        node->next = NULL;
    }

    return node; // finally
}

/* parent and child must not be NULL.
 */
int l4vfs_th_destroy_child(l4vfs_th_node_t * parent, l4vfs_th_node_t * child)
{
    l4vfs_th_node_t * temp, * current;
    int ret;

    current = parent->first_child;
    // check for the first node
    if (current == child)
    {
        temp = current->next;
        ret = l4vfs_th_free_node(current);
        if (ret)
        {
            LOG("Problem freeing node, ret = %d, ignored!", ret);
        }
        parent->first_child = temp;
        return 0;
    }

    // check for all other nodes
    while (current)
    {
        if (current->next == child)
        {
            temp = current->next;
            current->next = temp->next;
            ret = l4vfs_th_free_node(temp);
            if (ret)
            {
                LOG("Problem freeing node, ret = %d, ignored!", ret);
            }
            return 0;
        }
        current = current->next;
    }

    return 1;  // could not find the node
}

/* Does not free user data via free, as this might not have been
 * malloc()'ed.
 */
int l4vfs_th_free_node(l4vfs_th_node_t * node)
{
    int i;
    // remove from table
    for (i = 0; i < TREE_HELPER_MAX_OBJECTS; i++)
    {
        if (object_table[i] == node)
        {
            object_table[i] = NULL;
            break;
        }
    }
    if (i == TREE_HELPER_MAX_OBJECTS)  // no space left in table
    {
        LOG("Node not found in table, ignored!");
    }

    free(node->name);
    free(node);
    return 0;
}

// resolve long path
local_object_id_t l4vfs_th_resolve(local_object_id_t base, const char * name)
{
    char * remainder;
    local_object_id_t ret;

    // special case for absolute pathnames
    if (l4vfs_th_is_absolute_path(name))
    {
        remainder = l4vfs_th_get_remainder_path(name);
        ret = L4VFS_ROOT_OBJECT_ID;
    }
    else // normal case
    {
        remainder = strdup(name);
        ret = base;
    }

    // as long as we have something left in the path to resolve, go on
    while(strlen(remainder) > 0)
    {
        char * path, * temp;

        path = l4vfs_th_get_first_path(remainder);
        temp = l4vfs_th_get_remainder_path(remainder);

        ret = l4vfs_th_local_resolve(ret, path);

        free(remainder);
        free(path);
        remainder = temp;

        if (ret == L4VFS_ILLEGAL_OBJECT_ID)
        { // could not resolve, so free stuff and break out
            break;
        }
    };
    free(remainder);

    return ret;
}

// resolve one step
local_object_id_t l4vfs_th_local_resolve(local_object_id_t base,
                                         const char * name)
{
    l4vfs_th_node_t * node, * child;

    node = l4vfs_th_node_for_id(base);

    if (node == NULL)
        return L4VFS_ILLEGAL_OBJECT_ID;
    if (node->type != L4VFS_TH_TYPE_DIRECTORY)
        return L4VFS_ILLEGAL_OBJECT_ID;

    // fixme: is this really necessary
    if (strlen(name) == 0)  // could happen in "abc////xyz.txt"
        return base;
    if (strlen(name) == 1)
    {
        if (name[0] == '/')
            return L4VFS_ROOT_OBJECT_ID;
        else if (name[0] == '.')
            return base;
    }
    if (strlen(name) == 2)
        if (strcmp(name, "..") == 0)
        {
            if (node->parent)
                return node->parent->id;
            else
                return L4VFS_ILLEGAL_OBJECT_ID;
        }

    child = node->first_child;
    while (child)
    {
        if (strcmp(name, child->name) == 0)
            return child->id;
        child = child->next;
    }
    return L4VFS_ILLEGAL_OBJECT_ID;
}

char * l4vfs_th_rev_resolve(local_object_id_t base, local_object_id_t * parent)
{
    l4vfs_th_node_t * node;
    char * path, * ret, * temp;

    node = l4vfs_th_node_for_id(base);

    path = strdup("");
    // until we are at the root or parent
    while (node->parent != NULL || node->id == *parent)
    {
        ret = node->name;
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
        node = node->parent;
    }

    return path;
}

void l4vfs_th_init(void)
{
    int i;

    // reset object_table
    for (i = 0; i < TREE_HELPER_MAX_OBJECTS; i++)
    {
        object_table[i] = NULL;
    }
}

void l4vfs_th_dir_to_dirent(const l4vfs_th_node_t * node,
                            l4vfs_dirent_t * entry)
{
    entry->d_ino = node->id;
    entry->d_off = -1; // this is not really defined, linux kernel
                       // returnes slightly wrong number, at least for
                       // some FSs
    strncpy(entry->d_name, node->name, L4VFS_MAX_NAME);
    entry->d_name[L4VFS_MAX_NAME] = '\0';

    entry->d_reclen = l4vfs_th_dir_dirent_size(node);
}

int l4vfs_th_dir_dirent_size(const l4vfs_th_node_t * node)
{
    int len;
    l4vfs_dirent_t * temp;
    // + 1 for the '\0'
    len = sizeof(temp->d_ino) + sizeof(temp->d_off) +
          sizeof(temp->d_reclen) + strlen(node->name) + 1;
    // round up to align to word size
    len = (len + sizeof(int) - 1);
    len = (len / sizeof(int)) * sizeof(int);
    return len;
}

int l4vfs_th_dir_fill_dirents(const l4vfs_th_node_t * node, int index,
                              l4vfs_dirent_t * entry, int * length)
{
    int len, count = 0, i;
    l4vfs_th_node_t * actual, self, up;

    if (node == NULL)
    {
        LOG("NULL given to l4vfs_th_dir_fill_dirents");
        return -1;
    }
    if (node->type != L4VFS_TH_TYPE_DIRECTORY)
    {
        LOG("node with type != DIRECTORY given to l4vfs_th_dir_fill_dirents");
        return -1;
    }

    // '.'
    self.name = ".";
    if (index == 0)
    {
        len = l4vfs_th_dir_dirent_size(&self);
        if (count + len <= *length)
        {
            self.id = node->id;
            l4vfs_th_dir_to_dirent(&self, entry);
            entry = (l4vfs_dirent_t *)(((char *)entry) + entry->d_reclen);
            index++;
            count += len;
        }
        else
        {
            if (count == 0)
                *length = -1;
            else
                *length = count;
            return 0;
        }
    }

    // '..'
    up.name = "..";
    if (index == 1)
    {
        len = l4vfs_th_dir_dirent_size(&up);
        if (count + len <= *length)
        {
            if (node->parent)
                up.id = node->parent->id;
            else
                up.id = L4VFS_ILLEGAL_OBJECT_ID;

            l4vfs_th_dir_to_dirent(&up, entry);
            entry = (l4vfs_dirent_t *)(((char *)entry) + entry->d_reclen);
            index++;
            count += len;
        }
        else
        {
            if (count == 0)
                *length = -1;
            else
                *length = count;
            return 1;
        }
    }

    // search next relevant entry
    actual = node->first_child;
    for (i = 0; (actual != NULL) && (i + 2 < index);
         i++, actual = actual->next)
        ;
    if (actual == NULL)  // no more entries?
    {
        *length = count;
        return i + 2;
    }

    // now the normal dirs
    while (actual)
    {
        len = l4vfs_th_dir_dirent_size(actual);
        if (count + len <= *length)
        {
            l4vfs_th_dir_to_dirent(actual, entry);
            entry = (l4vfs_dirent_t *)(((char *)entry) + entry->d_reclen);
            index++;
            count += len;
        }
        else
        {
            // buffer full
            if (count == 0)
                *length = -1;
            else
                *length = count;
            return index;
        }
        actual = actual->next;
    }
    // EOF
    *length = count;
    return index;
}
