/**
 * \file   l4vfs/simple_file_server/server/state.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <string.h>
#include <stdlib.h>

#include "state.h"
#include "dirs.h"

#include <l4/log/l4log.h>
#include <l4/env/mb_info.h>

#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

#include "state.c.decl"

l4_ssize_t l4libc_heapsize = 1024 * 1024;

ARRAYLIST *files;
struct arraylist_services *arraylist;

static l4util_mb_mod_t *get_mod_by_index(int);

void state_init(void) {
    int i, j, size_static;
    char *name, *command, *temp;
    l4util_mb_mod_t *m;
    simple_file_t *s_file, *static_file;

    arraylist = &services;

    files = arraylist->create();

    //now include static linked files
    #include "state.c.inc"

    size_static = arraylist->size(files);

    for (i = 0; i < l4env_multiboot_info->mods_count; i++)
    {
        m=get_mod_by_index(i);

        s_file = (simple_file_t *) malloc(sizeof(simple_file_t));

        s_file->data = (l4_uint8_t *) m->mod_start;
        s_file->length = m->mod_end - m->mod_start;

        command = (char *)m->cmdline;

        LOGd(_DEBUG, "Command: '%s'!", command);

        /* find first occurence of space */
        name = strchr(command, ' ');
        if (name == NULL)
        {
            LOGd(_DEBUG, "No name found for '%s'!", command);
            name = command;
            temp = strrchr(name, '/');

            if (temp != NULL)
            {
                LOGd(_DEBUG, ".");
                name = temp + 1;
            }
        }
        else
        {
            LOGd(_DEBUG, "Found name '%s' for command '%s'.", name, command);
            name++; // skip the (hopefully) only space
        }

        // copy filename
        s_file->name = strdup(name);

        /* check if a static file with the same name exists already in list
         * our policy: overwrite static with dynamic loaded file
         */
        for (j=0; j < size_static; j++)
        {
            static_file = (simple_file_t *) arraylist->get_elem(files,j);

            if (strncmp(static_file->name, s_file->name, 
                strlen(s_file->name)) == 0)
            {
                arraylist->add_elem_at_index(files,s_file,j);
                break;  // there should be only one file with same name
            }
        }

        // check if we did not find static file with same name
        if (j == size_static)
        {
            arraylist->add_elem(files,s_file); // add dynamic loaded file
        }

    }

    if (_DEBUG)
    {
        for (i = 0; i < arraylist->size(files); i++)
        {
            s_file = (simple_file_t *) arraylist->get_elem(files, i);
            LOG("file at postion %d, name: %s", i, s_file->name);
        }
    }

    s_file->ds = NULL;

}

/*** REQUEST BINARY MODULE ***/
static l4util_mb_mod_t *get_mod_by_index(int idx)
{
    idx = idx % l4env_multiboot_info->mods_count;
    return (l4util_mb_mod_t *) (l4env_multiboot_info->mods_addr
        + (idx * sizeof(l4util_mb_mod_t)));
}
