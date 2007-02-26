/**
 * \file   dietlibc/lib/backends/select/src/select.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
/*** GENERAL INCLUDES ***/
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/util/util.h>
#include <l4/env/errno.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include <l4/log/l4log.h>
#include <l4/l4vfs/select_notify.h>
#include <l4/l4vfs/select_listener.h>
#include <l4/l4vfs/file-table.h>

/*** LOCAL INCLUDES ***/
#include "local.h"
#include "select_internal-client.h"

#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

static l4thread_t notif_t = L4THREAD_INVALID_ID;

static int select_create_info(int n, fd_set *readfds, fd_set *writefds,
                              fd_set *exceptfds, select_info_t *select_infos);

static int select_create_info(int n, fd_set *readfds, fd_set *writefds,
                              fd_set *exceptfds, select_info_t *select_infos)
{
    int mode = 0, i, j = 0;
    file_desc_t file_desc;
    select_info_t *current = select_infos, *prev;

    /* create select info struct */
    for (i=0; i < n && i < MAX_FILES_OPEN;i++)
    {
        if (ft_is_open(i))
        {
            mode = 0;

            file_desc = ft_get_entry(i);

            if (l4_is_invalid_id(file_desc.server_id))
                continue;

            if ((readfds) && FD_ISSET(i, readfds))
            {
                mode = SELECT_READ;
            }
            if ((writefds) && FD_ISSET(i, writefds))
            {
                mode |= SELECT_WRITE;
            }
            if ((exceptfds) && FD_ISSET(i, exceptfds))
            {
                mode |= SELECT_EXCEPTION;
            }

            /* if we have a hit fill a select info struct */
            if (mode)
            {
                current->local_fd = i;
                current->mode = mode;
                current->non_block_mode = 0;
                current->next = NULL;

                if (current != select_infos)
                {
                    prev = (current-1);
                    prev->next = current;
                }

                current++;
                j++;
            }
        }
   }

   return j;
}

static int select_notify(int type, select_info_t *select_infos,
                  l4_threadid_t notif_l4_tid);

static int select_notify(int type, select_info_t *select_infos,
                  l4_threadid_t notif_l4_tid)
{
    file_desc_t file_desc;
    select_info_t *current = select_infos;

    while (current)
    {
        file_desc = ft_get_entry(current->local_fd);

        if (l4_is_invalid_id(file_desc.server_id))
            goto next_current;

        switch (type)
        {
            case SELECT_REQUEST:    l4vfs_select_notify_request(
                                            file_desc.server_id,
                                            file_desc.object_handle,
                                            current->mode,
                                            notif_l4_tid);

                                     break;

            case SELECT_CLEAR:      l4vfs_select_notify_clear(
                                            file_desc.server_id,
                                            file_desc.object_handle,
                                            current->mode,
                                            notif_l4_tid);

                                     break;

                     default:        break;

        }
    next_current:
        current = current->next;
    }

    return 0;
}

int set_fd_set(fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
               select_info_t *select_infos)
{
    int max_n = 0;
    file_desc_t file_desc;
    fd_set *in = 0, *out = 0, *except = 0;
    select_info_t *current = select_infos;

    if (! select_infos)
    {
        return -1;
    }

    if (readfds)
    {
        in = calloc(sizeof(readfds), sizeof(fd_mask));

        if (! in)
            return -1;
    }

    if (writefds)
    {
        out = calloc(sizeof(writefds), sizeof(fd_mask));

        if (! out)
            return -1;
    }

    if (exceptfds)
    {
        except = calloc(sizeof(exceptfds), sizeof(fd_mask));

        if (! except)
            return -1;
    }

    while (current)
    {
        if (! ft_is_open(current->local_fd))
            goto next_current;

        file_desc = ft_get_entry(current->local_fd);

        if ((! l4_is_invalid_id(file_desc.server_id)) &&
            (current->non_block_mode &
            (SELECT_READ | SELECT_WRITE | SELECT_EXCEPTION)))
        {
            if ((readfds) && (current->non_block_mode & SELECT_READ) &&
                FD_ISSET(current->local_fd, readfds))
            {
                FD_SET(current->local_fd, in);
                max_n++;
            }
            if ((writefds) && (current->non_block_mode & SELECT_WRITE) &&
                FD_ISSET(current->local_fd, writefds))
            {
                FD_SET(current->local_fd, out);
                max_n++;
            }
            if ((exceptfds) && (current->non_block_mode & SELECT_EXCEPTION)
                && FD_ISSET(current->local_fd, exceptfds))
            {
                FD_SET(current->local_fd, except);
                max_n++;
            }
        }

    next_current:
        current = current->next;
    }

    if (max_n > 0)
    {
        if (readfds)
        {
            memcpy(readfds, in, sizeof(readfds));
        }
        if (writefds)
        {
            memcpy(writefds, out, sizeof(writefds));
        }
        if (exceptfds)
        {
            memcpy(exceptfds, except, sizeof(exceptfds));
        }
    }
    else
    {
        if (readfds)
        {
            FD_ZERO(readfds);
        }

        if (writefds)
        {
            FD_ZERO(writefds);
        }

        if (exceptfds)
        {
            FD_ZERO(exceptfds);
        }
    }

    free(in);
    free(out);
    free(except);

    return max_n;
}

int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
    struct timespec t;
    l4thread_t select_tid;
    l4_threadid_t notif_l4_tid;
    int ms = 0, ret;
    l4semaphore_t sem;
    CORBA_Environment env = dice_default_environment;
    select_info_t select_infos[MAX_FILES_OPEN];

    if (n < 0)
    {
        errno = EINVAL;
        return -1;
    }

    sem = L4SEMAPHORE_INIT(0);

    /* true means we use select to sleep a time period specified in timeout */
    if ((! readfds) && (! writefds) && (! exceptfds) && (timeout))
    {
        LOGd(_DEBUG, "fds_sets NULL, use select to sleep");

        t.tv_sec = timeout->tv_sec;
        t.tv_nsec = timeout->tv_usec / 1000;

        nanosleep(&t,&t);

        timeout->tv_sec = t.tv_sec;
        timeout->tv_usec = t.tv_nsec * 1000;

        return 0;
    }

    /* do we have a notify listener already? */
    if (notif_t == L4THREAD_INVALID_ID)
    {
        LOGd(_DEBUG,"try to create notify listener thread");

        /* create notify listener for our select thread */
        notif_t = l4thread_create((l4thread_fn_t) notify_listener_thread,
                                   NULL, L4THREAD_CREATE_SYNC);
        if (notif_t < 0)
        {
            errno = ENOMEM;
            return -1;
        }
    }

    notif_l4_tid = l4thread_l4_id(notif_t);

    if (timeout)
    {
        ms = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
    }

    /* we create a list of selected file info structures to filter out
     * some wrong fds, we also need this list for register at notify
     * listener */
    ret = select_create_info(n, readfds, writefds, exceptfds, select_infos);

    /* nothing to send, because only wrong fds */
    if (! ret)
    {
        errno = EBADF;
        return -1;
    }

    select_tid = l4thread_myself();

    /* register at notify listener thread
     * as from now the listener saves incoming notifications
     * this means that fds are non blocking on special modes */
    select_internal_register_call(&notif_l4_tid, select_tid,
                                  (l4_addr_t) select_infos,
                                  (l4_addr_t) &sem, &env);

    /* send notify request messages to each dedicated l4vfs server */
    ret = select_notify(SELECT_REQUEST, select_infos, notif_l4_tid);

    /* NULL reference of timeout -> POSIX select waits indefinitely */
    if (! timeout)
    {
        l4semaphore_down(&sem);
    }
    else
    {
       /* if notify listener did not get a notification during our
        * select_notify operation we wait ms for a semaphore up
        * made by select notify listener */
        ret = l4semaphore_down_timed(&sem, ms);
    }

    /* now we have to deregister at notify listener
     * by now the listener handles notifications as bogus messages */
    select_internal_deregister_call(&notif_l4_tid, select_tid, &env);

    /* send notify clear messages to each dedicated l4vfs server */
    ret = select_notify(SELECT_CLEAR, select_infos, notif_l4_tid);

    /* fill fd_sets with selected file descriptors or fill them with zeros */
    ret = set_fd_set(readfds, writefds, exceptfds, select_infos);

    return ret;
}


