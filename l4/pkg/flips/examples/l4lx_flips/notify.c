/**
 * \file   flips/examples/l4lx_flips/notify.c
 * \brief  Notify implementation for L4LX_FLIPS
 *
 * \date   02/03/2006
 * \author Jens Syckor <js712688@inf.tu-dresden.de>
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/*
 * The new select implementation uses a thread that executes select() for the
 * whole fd_set of one session in an infinite loop. fd_set changes are
 * propagated by signalling SIGHUP to this thread, which restarts the loop
 * afterwards.
 *
 * Now each connection has one "session thread" and one "select thread" from
 * beginning. -- Christian.
 */

/*** GENERAL INCLUDES ***/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/l4vfs/select_listener.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>

/*** LOCAL INCLUDES ***/
#include "local.h"

/*** NOTIFY SELECT THREAD ***/
void notify_select_thread(struct notify_fd_set *fds)
{
	fd_set *readfds = &fds->readfds;
	fd_set *writefds = &fds->writefds;
	fd_set *exceptfds = &fds->exceptfds;

	for (;;) {
		int ret = 0;

		/* use POSIX select */
		ret = select(FD_SETSIZE, readfds, writefds, exceptfds, NULL);

		if (ret < 0) {
			/* restart signal */
			if (errno == EINTR)
				continue;
			/* XXX what else to do here? */
			perror("select");
		}

		int fd;
		int mode = 0;
		for (fd = 0; fd < FD_SETSIZE; fd++) {

			if (FD_ISSET(fd, readfds)) mode |= SELECT_READ;
			if (FD_ISSET(fd, writefds)) mode |= SELECT_WRITE;
			if (FD_ISSET(fd, exceptfds)) mode |= SELECT_EXCEPTION;

			/* notify if something happened */
			if (!mode)
				continue;

			l4vfs_select_listener_send_notification(fds->notifier,
			                                        fd,
			                                        mode);
		}
	}
}

void notify_request(struct notify_fd_set *fds,
                    int fd, int mode, const l4_threadid_t notifier)
{
	if (!l4_is_nil_id(fds->notifier) && !l4_is_invalid_id(fds->notifier))
		if (!l4_thread_equal(fds->notifier, notifier)) {
			D("supporting only 1 notifier ("l4util_idfmt"/"l4util_idfmt")",
			  l4util_idstr(fds->notifier), l4util_idstr(notifier));
		}

	fds->notifier = notifier;

	/* set file descriptor according to mode */
	if (mode & SELECT_READ)
		FD_SET(fd, &fds->readfds);

	if (mode & SELECT_WRITE)
		FD_SET(fd, &fds->writefds);

	if (mode & SELECT_EXCEPTION)
		FD_SET(fd, &fds->exceptfds);
}

void notify_clear(struct notify_fd_set *fds,
                  int fd, int mode, const l4_threadid_t notifier)
{
	/* clear file descriptor according to mode */
	if (mode & SELECT_READ)
		FD_CLR(fd, &fds->readfds);

	if (mode & SELECT_WRITE)
		FD_CLR(fd, &fds->writefds);

	if (mode & SELECT_EXCEPTION)
		FD_CLR(fd, &fds->exceptfds);
}

void notify_init_fd_set(struct notify_fd_set *fds)
{
	FD_ZERO(&fds->readfds);
	FD_ZERO(&fds->writefds);
	FD_ZERO(&fds->exceptfds);
}
