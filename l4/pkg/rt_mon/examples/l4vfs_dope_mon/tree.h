#ifndef __RT_MON_EXAMPLES_L4VFS_DOPE_MON_TREE_H_
#define __RT_MON_EXAMPLES_L4VFS_DOPE_MON_TREE_H_

#define TREE_WIDTH 11

enum node_type_e {NODE_LEAF, NODE_DIR, NODE_UNKNOWN};

typedef struct tree_node_s
{
    char * name;
    int    id;
    int    expanded;
    int    type;
    struct tree_node_s * parent;
    struct tree_node_s * next;
    struct tree_node_s * first_child;
} tree_node_t;


tree_node_t * tree_node_create(const char * name, int id, int type);
void tree_node_remove_childs(tree_node_t * node);
void tree_node_add_child_sorted(tree_node_t * parent, tree_node_t * child);
void tree_node_create_dope_repr(const tree_node_t * node);
void tree_node_set_expanded(tree_node_t * node, int exp);
int tree_node_get_expanded(tree_node_t * node);
void tree_node_populate_from_dir(tree_node_t * node, char * name, int depth);
char * tree_node_get_path(tree_node_t * node);

#endif
