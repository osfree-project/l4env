/**
 * \file   l4vfs/include/tree_helper.h
 * \brief  
 *
 * \date   10/25/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_INCLUDE_TREE_HELPER_H_
#define __L4VFS_INCLUDE_TREE_HELPER_H_

#include <l4/l4vfs/types.h>

enum object_type_e {L4VFS_TH_TYPE_OBJECT, L4VFS_TH_TYPE_DIRECTORY};

typedef struct l4vfs_th_node_s
{
    int                      type;         // dir, object, ...
    struct l4vfs_th_node_s * next;         // next object in same dir
    struct l4vfs_th_node_s * first_child;  // isn't it obvious ?
    struct l4vfs_th_node_s * parent;       // parent node in tree
    local_object_id_t        id;           // object identifier
    char                   * name;         // objects name
    void                   * data;         // pointer to user data
    int                      usage_count;  // how many refs. open
} l4vfs_th_node_t;

EXTERN_C_BEGIN

l4vfs_th_node_t * l4vfs_th_new_node(const char * name, int type,
                                    l4vfs_th_node_t * parent,
                                    void * data);
/* Destroys a node in the tree.
 *
 * The node is free()'ed and it is removed from the tree (all links are
 * bridged)
 *
 * parent and child must not be NULL.
 */
int               l4vfs_th_destroy_child(l4vfs_th_node_t * parent,
                                         l4vfs_th_node_t * child);

/* Dispose memory for a node, remove it from the node table.
 *
 * Does not free user data via free, as this might not have been
 * malloc()'ed.  This must be done externally.
 */
int               l4vfs_th_free_node(l4vfs_th_node_t * node);

l4vfs_th_node_t * l4vfs_th_child_for_name(l4vfs_th_node_t * parent,
                                          const char * name);
l4vfs_th_node_t * l4vfs_th_node_for_id(local_object_id_t id);
local_object_id_t l4vfs_th_resolve(local_object_id_t base, const char * name);
local_object_id_t l4vfs_th_local_resolve(local_object_id_t base,
                                         const char * name);
char * l4vfs_th_rev_resolve(local_object_id_t base, local_object_id_t * parent);
void   l4vfs_th_init(void);
void   l4vfs_th_dir_to_dirent(const l4vfs_th_node_t * node,
                              l4vfs_dirent_t * entry);
int    l4vfs_th_dir_dirent_size(const l4vfs_th_node_t * node);

/**
 * @brief Convert parts of a directory node to a dirent buffer
 *
 * @param  node   tree node to operate on (should be a directory)
 * @param  index  file number to begin with conversion (0 = '.', 1 =
 *                '..', 2 = '[first file]', ...)
 * @param  entry  pointer to memory buffer to fill

 * @retval length [in] size of entry in bytes
 *                - [out] number bytes written to entry (should be <= [in])
 *                - -1 in case of 0 bytes could be written
 * @return index to operate on with the next call
 *         - -1 in case an invalid node is given
 */
int    l4vfs_th_dir_fill_dirents(const l4vfs_th_node_t * node, int index,
                                 l4vfs_dirent_t * entry, int * length);

int    l4vfs_th_is_absolute_path(const char * path);
char * l4vfs_th_get_first_path(const char * name);
char * l4vfs_th_get_remainder_path(const char * name);

EXTERN_C_END

#endif
