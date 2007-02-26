/**
 * \file   l4vfs/name_server/server/dirs.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_NAME_SERVER_SERVER_DIRS_H_
#define __L4VFS_NAME_SERVER_SERVER_DIRS_H_

#include <l4/l4vfs/types.h>

#define NAME_SERVER_MAX_DIRS 1024

enum object_type {OBJ_DIR, OBJ_FILE};

typedef struct obj_s
{
    char *           name;
    struct obj_s *   parent;
    struct obj_s *   next;
    struct obj_s *   prev;
    enum object_type type;
    union spec_u
    {
        struct dir_s
        {
            struct obj_s * first_child;   // pointer to first child, or
            local_object_id_t object_id;
        } dir;
        struct file_s
        {
            object_id_t    oid;           // reference to external object
        } file;
    } spec;
} obj_t;

extern obj_t root;

void    init_dirs(int create_examples);
int     dir_add_child_dir(obj_t * parent, const char * name);
int     dir_add_child_file(obj_t * parent, const char * name, object_id_t oid);
obj_t * dir_get_child(obj_t * dir, const char * name);
local_object_id_t
        get_next_object_id(void);
int     check_dir_name(obj_t * parent, const char * name);
obj_t * map_get_dir(local_object_id_t object_id);
void    map_insert_dir(obj_t * dir);
void    print_tree(int indent, obj_t * dir);
void    dir_to_dirent(obj_t * dirp, l4vfs_dirent_t * entry);
int     dir_dirent_size(obj_t * dirp);
int     dir_fill_dirents(obj_t * dirp, int index,
                     l4vfs_dirent_t* entry, int * length);
int dir_is_dir(obj_t * obj);

#endif
