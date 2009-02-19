/**
 * \file   l4vfs/lib/libc_backends/select/src/select_listener.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
/*** GENERAL INCLUDES_t ***/
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <l4/l4vfs/file-table.h>
#include <l4/l4vfs/select_listener-server.h>

/*** LOCAL INCLUDES ***/
#include "local.h"
#include "select_internal-server.h"

#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

/* info structure of select notify listener */
typedef struct listener_info
{
    struct listener_info *next;     /* next listener info structure */
    l4thread_t select_tid;          /* id of belonging select thread */
    select_info_t *sel_info;        /* list of selected files */
    l4semaphore_t *sem;             /* semaphore of belonging select thread */
} listener_info_t;

/* pointer to first listener info struct */
static listener_info_t *listener_info_start;

/* start notify listener thread and go into server loop */
void notify_listener_thread(void)
{
    void *ret_val = NULL;
    CORBA_Server_Environment env = dice_default_server_environment;

    /* we are up */
    if (l4thread_started(ret_val) < 0)
    {
        LOG_Error("Startup notification of notify listener failed");
        return;
    }

    /* go into server loop and wait for notifications */
    select_internal_server_loop(&env);
}

l4_int32_t
l4vfs_select_listener_send_notification_component(CORBA_Object _dice_corba_obj,
                                                  object_handle_t fd,
                                                  l4_int32_t mode,
                                                  CORBA_Server_Environment *_dice_corba_env)
{
    listener_info_t *current;
    select_info_t *curr_select;
    file_desc_t fd_s;

    if (fd < 0)
    {
        LOG_Error("no valid file descriptor, fd %d",fd);
        return -EBADF;
    }

    if (! (mode & (SELECT_READ | SELECT_WRITE | SELECT_EXCEPTION)))
    {
        LOG_Error("unknown operation mode %d",mode);
        return -EINVAL;
    }

    current = listener_info_start;

    if (! current)
    {
        return -EFAULT;
    }

    /* search listener info structure which contains selected fd */
    while (current)
    {
        curr_select = current->sel_info;

        while (curr_select)
        {

            fd_s = ft_get_entry(curr_select->local_fd);

            if (l4_task_equal(fd_s.server_id, *_dice_corba_obj)
                 && fd_s.object_handle == fd
                 && (curr_select->mode & mode))
            {
                /* we have found selected fd, save non blocking mode
                 * and increment semaphore to wakeup select thread */
                curr_select->non_block_mode = mode;
                l4semaphore_up(current->sem);

                LOGd(_DEBUG, "found fd: %d, sem->counter: %d", fd_s.object_handle,
                              current->sem->counter);

                break;
            }

            curr_select = curr_select->next;
        }

        current = current->next;
    }

    return 0;
}

l4_int32_t
select_internal_deregister_component(CORBA_Object _dice_corba_obj,
                                     l4thread_t select_tid,
                                     CORBA_Server_Environment *_dice_corba_env)
{
    listener_info_t *current, *prev;

    current = listener_info_start;
    prev = listener_info_start;

    if (! current)
    {
        return -EFAULT;
    }

    while (current)
    {
        if (l4thread_equal(current->select_tid, select_tid))
        {
            if (listener_info_start == current)
            {
                listener_info_start = current->next;
            }
            else
            {
                prev->next = current->next;
            }

            free(current);
            break;
        }

        prev = current;
        current = current->next;
    }

    return 0;
}

l4_int32_t
select_internal_register_component(CORBA_Object _dice_corba_obj,
                                   l4thread_t select_tid,
                                   l4_addr_t fd_set,
                                   l4_addr_t select_sem,
                                   CORBA_Server_Environment *_dice_corba_env)
{
    listener_info_t *current;

    if (! fd_set)
    {
        LOG_Error("fd_set NULL");
        return -EFAULT;
    }

    if (! select_sem)
    {
        LOG_Error("select semaphore NULL");
        return -EFAULT;
    }

    current = (listener_info_t *) malloc(sizeof(listener_info_t));

    if (! current)
    {
        return -ENOMEM;
    }

    /* created new listener info struct and save data */
    current->select_tid = select_tid;
    current->sel_info   = (select_info_t *) fd_set;
    current->sem        = (l4semaphore_t *) select_sem;
    current->next       = listener_info_start;

    listener_info_start = current;

    return 0;
}
