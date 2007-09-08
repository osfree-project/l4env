/**
 * \file   l4vfs/term_server/lib/vt100/select.c
 * \brief  select() implementation
 *
 * \date   09/03/2007
 * \author Bjoern Doebel <doebel@tudos.org>
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>

#include <l4/log/l4log.h>
#include <l4/util/macros.h>

#include "lib.h"
#include "keymap.h"

extern int _DEBUG;

void vt100_set_select_info(termstate_t *term, object_handle_t handle,
                          int mode, const l4_threadid_t *notify_handler)
{
    if (!l4_is_invalid_id(term->select_handler))
    {
        LOG("Cannot overwrite existing select handler "l4util_idfmt" with "
            l4util_idfmt, l4util_idstr(term->select_handler), l4util_idstr(*notify_handler)
        );
    }

    term->select_handler    = *notify_handler;
    term->select_mode       = mode;
    term->select_fd         = handle;
}


void vt100_unset_select_info(termstate_t *term)
{
    term->select_handler    = L4_INVALID_ID;
    term->select_mode       = 0;
    term->select_fd         = -1;
}


void vt100_select_notify(termstate_t *term)
{
    if (!l4_is_invalid_id(term->select_handler))
        l4vfs_select_listener_send_notification(term->select_handler,
                                                term->select_fd,
                                                term->select_mode);
}


int vt100_data_avail(termstate_t *term)
{
    int ret;

    return (term->keylist_next_write != term->keylist_next_read);
}
