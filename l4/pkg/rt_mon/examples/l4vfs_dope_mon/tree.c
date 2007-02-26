#include "tree.h"
#include "main.h"

#include <l4/dope/dopelib.h>
#include <l4/dope/vscreen.h>

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>


/* Create a now tree_node, set some default values.
 */
tree_node_t * tree_node_create(const char * name, int id, int type)
{
    tree_node_t * temp;

    temp = malloc(sizeof(tree_node_t));
    temp->name        = strdup(name);
    temp->id          = id;
    temp->type        = type;
    temp->expanded    = 0;
    temp->next        = NULL;
    temp->first_child = NULL;
    temp->parent      = NULL;

    return temp;
}

/* Free all childs of a node.
 */
void tree_node_remove_childs(tree_node_t * node)
{
    tree_node_t * child, * temp;

    child = node->first_child;
    while (child)
    {
        free(child->name);
        temp = child->next;
        tree_node_remove_childs(child);
        free(child);
        child = temp;
    }
    node->first_child = NULL;
}

/* Set the expanded flag for a node.
 */
void tree_node_set_expanded(tree_node_t * node, int exp)
{
    node->expanded = exp;
}

/* Get the expanded flag for a node.
 */
int tree_node_get_expanded(tree_node_t * node)
{
    return node->expanded;
}

void tree_node_create_dope_repr(const tree_node_t * node)
{
    int id, child_num;
    tree_node_t * current;
    char cmd[200];
    id = node->id;

    // container
    dope_cmdf(app_id, "tree_container_%d = new Grid()", id);

    // header
    dope_cmdf(app_id, "tree_header_%d = new Grid()", id);
    dope_cmdf(app_id, "tree_container_%d.place(tree_header_%d,"
                      " -column 1 -row 0 -align wns)", id, id);
    // label
    dope_cmdf(app_id, "tree_name_%d = new Label()", id);
    dope_cmdf(app_id, "tree_name_%d.set(-text \"%s\")", id, node->name);
    dope_cmdf(app_id, "tree_header_%d.place(tree_name_%d, -column 1 -row 0)",
              id, id);
    
    // icon
    dope_cmd(app_id, "temp = new VScreen()");
    dope_cmdf(app_id, "temp.set(-fixw %d -fixh %d)", 16, 16);
    if (node->type == NODE_DIR)
        dope_cmd(app_id, "temp.share(folder_grey)");
    else if (node->type == NODE_LEAF)
        dope_cmd(app_id, "temp.share(misc)");
    else
        dope_cmd(app_id, "temp.share(unknown)");

    dope_cmdf(app_id, "tree_header_%d.place(temp, -column 0 -row 0)", id);

    snprintf(cmd, 200, "tree_header_%d", id);
    dope_bind(app_id, cmd, "press", press_callback,
              (void *)(node));

    // header ruler
    dope_cmd(app_id, "temp = new VScreen()");
    // fixme: consts
    dope_cmdf(app_id, "temp.set(-fixw %d -fixh %d)", TREE_WIDTH, 9);
    // ok, now its a bit tricky: we have to distinguish some cases here
    if (node->first_child == NULL)            // leaf node
    {
        if (node->next == NULL)               // last node
            dope_cmd(app_id, "temp.share(tree_corner)");
        else
            dope_cmd(app_id, "temp.share(tree_cross)");
    }
    else                                      // non-leaf node
    {
        if (node->expanded == 0)              // '+' non-leaf node
        {
            dope_cmd(app_id, "temp.share(tree_plus)");
            dope_bind(app_id, "temp", "press", press_callback, (void *)(node));
        }
        else                                  // '-' non-leaf node
        {
            dope_cmd(app_id, "temp.share(tree_minus)");
            dope_bind(app_id, "temp", "press", press_callback, (void *)(node));
        }
    }
    // pad the ruler with vert elements
    dope_cmd(app_id, "tempg = new Grid()");
    dope_cmdf(app_id, "tree_container_%d.place(tempg, -column 0 -row 0)", id);

    // the ruler itself ...
    dope_cmd(app_id, "tempg.place(temp, -column 0 -row 1)");

    // ... above ...
    dope_cmd(app_id, "temp = new VScreen()");
    dope_cmdf(app_id, "temp.set(-fixw %d)", TREE_WIDTH);
    dope_cmd(app_id, "temp.share(tree_vert)");
    dope_cmd(app_id, "tempg.place(temp, -column 0 -row 0)");

    // ... below
    dope_cmd(app_id, "temp = new VScreen()");
    dope_cmdf(app_id, "temp.set(-fixw %d)", TREE_WIDTH);
    if (node->next == NULL)           // last node
        dope_cmd(app_id, "temp.share(tree_none)");
    else
        dope_cmd(app_id, "temp.share(tree_vert)");
    dope_cmd(app_id, "tempg.place(temp, -column 0 -row 2)");

    if (node->expanded == 0 || node->first_child == NULL)
        return; // leaf node? exit here!

    // childs and ruler elements
    for (current = node->first_child, child_num = 0;
         current != NULL;
         current = current->next, child_num ++)
    {
        // ruler for child
        dope_cmd(app_id, "temp = new VScreen()");
        dope_cmdf(app_id, "temp.set(-fixw %d)", TREE_WIDTH);
        if (node->next == NULL)           // last node
            dope_cmd(app_id, "temp.share(tree_none)");
        else
            dope_cmd(app_id, "temp.share(tree_vert)");
        dope_cmdf(app_id, "tree_container_%d.place(temp, -column 0 -row %d)",
                  id, child_num + 1);

        // create and add child representation
        tree_node_create_dope_repr(current);
        dope_cmdf(app_id, "tree_container_%d.place(tree_container_%d,"
                          " -column 1 -row %d)",
                  id, current->id, child_num + 1);
    }
}

void tree_node_add_child_sorted(tree_node_t * parent, tree_node_t * child)
{
    child->parent = parent;

    // we are the first child
    if (parent->first_child == NULL)
    {
        parent->first_child = child;
    }
    else  // there are others allready
    {
        tree_node_t * temp;

        temp = parent->first_child;
        if (strcmp(child->name, temp->name) < 0)  // we come first
        {
            child->next = temp;
            parent->first_child = child;
            return;
        }
        while (temp)
        {
            if (temp->next == NULL)
            {
                temp->next = child;
                return;
            }
            if (strcmp(child->name, temp->name) >= 0 &&
                strcmp(child->name, temp->next->name) < 0)
            {
                child->next = temp->next;
                temp->next = child;
                return;
            }
            temp = temp->next;
        }
    }
}

void tree_node_populate_from_dir(tree_node_t * node, char * name, int depth)
{
    DIR * D;
    struct dirent * dir;
    tree_node_t * temp = NULL;
    char * temp_name;
    int ret, type;
    struct stat sb;

    if (depth <= 0)
        return;

    D = opendir(name);
    if (D == NULL)
        return;
    while ((dir = readdir(D)))
    {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
            continue;

        // compute abs. path
        temp_name = malloc(strlen(name) + strlen(dir->d_name) + 2);
        strcat(temp_name, name);
        strcat(temp_name, "/");
        strcat(temp_name, dir->d_name);

        // check if dir
        ret = stat(temp_name, &sb);
        if (ret != 0)
            type = NODE_UNKNOWN;
        else if (S_ISDIR(sb.st_mode))
            type = NODE_DIR;
        else
            type = NODE_LEAF;

        temp = tree_node_create(dir->d_name, counter++, type);
        tree_node_add_child_sorted(node, temp);

        // descend if possible
        tree_node_populate_from_dir(temp, temp_name, depth - 1);
        free(temp_name);
    }
    closedir(D);
}

char * tree_node_get_path(tree_node_t * node)
{
    tree_node_t * parent;
    char * name, * temp;
    int name_length;

    // compute length for pathstring
    name_length = strlen(node->name) + 2;
    parent = node->parent;
    while (parent != NULL)
    {
        name_length += strlen(parent->name) + 1;
        parent = parent->parent;
    }

    // construct pathstring
    name = malloc(name_length);
    temp = malloc(name_length);
    strcpy(name, "/");
    strcat(name, node->name);
    parent = node->parent;
    while (parent != NULL)
    {
        strcpy(temp, "/");
        strcat(temp, parent->name);
        strcat(temp, name);
        strcpy(name, temp);
        parent = parent->parent;
    }
    free(temp);
    return name;
}

