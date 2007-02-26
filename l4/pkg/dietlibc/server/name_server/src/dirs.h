#ifndef __DIETLIBC_LIB_SERVER_NAME_SERVER_SRC_DIRS_H_
#define __DIETLIBC_LIB_SERVER_NAME_SERVER_SRC_DIRS_H_

#include <l4/dietlibc/io-types.h>

#include <dirent.h>

#define NAME_SERVER_MAX_DIRS 1024

typedef struct dir_s
{
    struct dir_s * parent;
    struct dir_s * next;
    struct dir_s * prev;
    struct dir_s * first_child;
    char * name;
    local_object_id_t object_id;
} dir_t;

extern dir_t root;

void init_dirs(void);
int dir_add_child(dir_t * parent, const char * name);
dir_t * dir_get_child(dir_t * dir, const char * name);
local_object_id_t get_next_object_id(void);
int check_dir_name(dir_t * parent, const char * name);
dir_t * map_get_dir(local_object_id_t object_id);
void map_insert_dir(dir_t * dir);
void print_tree(int indent, dir_t * dir);
void dir_to_dirent(dir_t * dirp, struct dirent * entry);
int dir_dirent_size(dir_t * dirp);
int dir_childs_dirent_size(dir_t * dirp);
void dir_childs_to_dirents(dir_t * dirp, struct dirent ** entry, int * length);

#endif
