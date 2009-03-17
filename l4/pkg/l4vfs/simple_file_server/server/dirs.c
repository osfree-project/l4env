/**
 * \file   l4vfs/simple_file_server/server/dirs.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "state.h"
#include "dirs.h"
#include "resolve.h"

#include <l4/l4vfs/types.h>

#include <l4/log/l4log.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern int _DEBUG;

int dirent_size(char *fname)
{
    int len;
    l4vfs_dirent_t * temp;
    // + 1 for the '\0'
    len = sizeof(temp->d_ino) + sizeof(temp->d_off) +
          sizeof(temp->d_reclen) + strlen(fname) + 1;
    // round up to align to word size
    len = (len + sizeof(int) - 1);
    len = (len / sizeof(int)) * sizeof(int);
    return len;
}

void convert_to_dirent(int object_id, char *fname, l4vfs_dirent_t * entry)
{

    entry->d_ino = object_id;
    entry->d_off = -1; // this is not really defined, linux kernel
                       // returnes slightly wrong number, at least for
                       // some FSs
    strncpy(entry->d_name, fname, L4VFS_MAX_NAME);
    entry->d_name[L4VFS_MAX_NAME] = '\0';

    entry->d_reclen = dirent_size(fname);
}

int fill_dirents(int index, l4vfs_dirent_t * entry, int * length)
{
    int len, count = 0;
    simple_file_t *actual;
    actual = NULL;

    // '.'
    if (index == 0)
    {
        len = dirent_size(".");

        if (count + len <= *length)
        {
            convert_to_dirent(0, ".", entry);
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
        len = dirent_size("..");

        if (count + len <= *length)
        {
            // fixme: this is evil, you should not use '0' here
            convert_to_dirent(0, "..", entry);
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

    // because self and up are not file list we have to change index value
    // up to now index is the object id
    for (;;) 
    {
        actual = (simple_file_t *) arraylist->get_elem(files,index - 2);

        if (actual == NULL) break;

        len = dirent_size(actual->name);

        if (count + len <= *length)
        {
            convert_to_dirent(index - 1, actual->name, entry);
            entry = (l4vfs_dirent_t *)(((char *)entry) + entry->d_reclen);
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
        index++;
    }

    // EOF
    * length = count;

    return index;
}
