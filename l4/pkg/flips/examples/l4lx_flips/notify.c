/*
 * \brief   Notify implementation for L4LX_FLIPS
 * \date    2004-10-26
 * \author  Jens Syckor <js712688@os.inf.tu-dresden.de>
 */

/*** GENERAL INCLUDES ***/
#include <stdlib.h>

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/l4vfs/select_listener.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>

/*** LOCAL INCLUDES ***/
#include "local.h"

void notify_session_thread(void *arg);

/** notify session thread, started for every notify_request **/
void notify_session_thread(void *arg)
{
	thread_entry_t *entry = (thread_entry_t *) arg;
	fd_set readfds, writefds, exceptfds;
	notif_private_t *notif;
	int res = 0, non_block_mode = 0;

	if (! entry)
		return;

	notif = (notif_private_t *) tt_entry_get_private_data(entry);

	if (! notif)
		return;

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);

	/* check mode for selected non-blocking operations */
	if (notif->mode & SELECT_READ)
	{
		FD_SET(entry->fd, &readfds);
	}

	if (notif->mode & SELECT_WRITE)
	{
		FD_SET(entry->fd, &writefds);
	}

	if (notif->mode & SELECT_EXCEPTION)
	{
		FD_SET(entry->fd, &exceptfds);
	}

	/* send startup to thread that created us */
	l4thread_started(NULL);

	/* use POSIX select */
	res = select(entry->fd + 1, &readfds, &writefds, &exceptfds, NULL);

	/* true -> select returned without a non-blocking fd */
	if (! res)
		goto out;

	if (FD_ISSET(entry->fd, &readfds))
	{
		non_block_mode = SELECT_READ;
	}

	if (FD_ISSET(entry->fd, &writefds))
	{
		non_block_mode |= SELECT_WRITE;
	}

	if (FD_ISSET(entry->fd, &exceptfds))
	{
		non_block_mode |= SELECT_EXCEPTION;
	}

	if (! non_block_mode)
		goto out;

	/* send message to notify thread */
	l4vfs_select_listener_send_notification(notif->notif_tid,
			                                entry->fd,
			                                non_block_mode);
out:
	entry->worker_tid = L4_INVALID_ID;
}

void internal_notify_request(int fd, int mode, l4_threadid_t * owner_tid,
		                     l4_threadid_t *notify_tid)
{
	int i;
	l4thread_t tid;
	thread_entry_t entry;
	notif_private_t notif;

	entry = tt_get_entry(fd, *owner_tid);

	if (entry.fd == -1)
	{
		/* we have to fill a new thread_entry_t element */
		i = tt_get_next_free_entry();

		/* true -> no free worker thread */
		if (i == -1)
			return;

		notif.mode = mode;

		entry.owner_tid = *owner_tid;
		entry.fd = fd;

		notif.notif_tid = *notify_tid;
		tt_entry_set_private_data(&entry, &notif);
	}
	else
	{
		/* we have to update a thread_entry_t element */
		i = tt_get_entry_id(fd, *owner_tid);

		if (! l4_is_invalid_id(entry.worker_tid))
		{
			l4thread_shutdown(l4thread_id(entry.worker_tid));
			entry.worker_tid = L4_INVALID_ID;
		}

		notif = *((notif_private_t *) tt_entry_get_private_data(&entry));
		notif.mode |= mode;
		notif.notif_tid = *notify_tid;
	
		tt_entry_set_private_data(&entry, &notif);
	}

	/* create worker thread */
	tid = l4thread_create((l4thread_fn_t) notify_session_thread,
			               &entry, L4THREAD_CREATE_SYNC);

	if (tid <= 0)
	{
		LOG_Error("Could not create thread, res (%d)", tid);
		return;
	}

	entry.worker_tid = l4thread_l4_id(tid);
	tt_fill_entry(i, entry);
}

void internal_notify_clear(int fd, int mode, l4_threadid_t *owner_tid,
		                   l4_threadid_t *notify_tid)
{
	thread_entry_t entry;

	entry = tt_get_entry(fd, *owner_tid);

	if (entry.fd == -1)
	{
		LOG("Unknown fd (%d)", fd);
		return;
	}

	/* if true, thread finished its work before */
	if (! l4_is_invalid_id(entry.worker_tid))
	{
		l4thread_shutdown(l4thread_id(entry.worker_tid));
	}

	free(tt_entry_get_private_data(&entry));

	tt_free_entry(fd, *owner_tid);
}
