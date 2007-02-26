/**
 * \file   l4vfs/simple_file_server/server/arraylist.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4VFS_SIMPLE_FILE_SERVER_SERVER_ARRAYLIST_H_
#define __L4VFS_SIMPLE_FILE_SERVER_SERVER_ARRAYLIST_H_

#ifndef ARRAYLIST
#define ARRAYLIST void
#endif

struct arraylist_services  {
    ARRAYLIST *(*create)                (void);
    void       (*destroy)               (ARRAYLIST *al);
    void       (*add_elem)              (ARRAYLIST *al,void *value);
    void      *(*get_elem)              (ARRAYLIST *al,int index);
    void       (*remove_elem)           (ARRAYLIST *al,int index);
    void      *(*get_first)             (ARRAYLIST *al);
    void      *(*get_last)              (ARRAYLIST *al);
    void      *(*get_next)              (ARRAYLIST *al);
    void      *(*get_prev)              (ARRAYLIST *al);
    int        (*is_empty)              (ARRAYLIST *al);
    int        (*size)                  (ARRAYLIST *al);
    void       (*set_iterator)          (ARRAYLIST *al);
    int        (*get_current_index)     (ARRAYLIST *al);
    void       (*add_elem_at_index)     (ARRAYLIST *al,void *value, int index);
};

#endif  // __L4VFS_SIMPLE_FILE_SERVER_SERVER_ARRAYLIST_H_
