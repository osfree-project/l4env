#include <l4/log/l4log.h>

#include <stdlib.h>
#include <string.h>

#include "helper.h"

/* Checks the usage count of a node and cleans up accordingly.
 */
void _check_and_clean_node(l4vfs_th_node_t * node)
{
    int ret;
    l4vfs_th_node_t * parent;

    // not used and no childs
    if (node->usage_count <= 0 && node->first_child == NULL)
    {
        parent = node->parent;
        if (parent == NULL)
            return;
        ret = l4vfs_th_destroy_child(parent, node);
        if (ret)
        {
            LOG("destroying child failed, ignored!");
        }

        _check_and_clean_node(parent);
    }
}


char * _version_name(char * name)
{
    char * temp;

    temp = malloc(strlen(name) + 2);
    strcpy(temp, name);
    free(name);
    strcat(temp, "'");
    return temp;
}
