/**
 * \file   dietlibc/lib/backends/select/src/local.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __DIETLIBC_LIB_BACKENDS_SELECT_SRC_LOCAL_H_
#define __DIETLIBC_LIB_BACKENDS_SELECT_SRC_LOCAL_H_
#define SELECT_REQUEST   1
#define SELECT_CLEAR     2

typedef struct select_info
{
    int local_fd;
    int mode;                 /* selected operations for non blocking */
    int non_block_mode;       /* currently possible non blocking operations */
    struct select_info *next; /* pointer to next select info structure */
} select_info_t;

int set_fd_set(fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
               select_info_t *select_infos);

void notify_listener_thread(void);

#endif
