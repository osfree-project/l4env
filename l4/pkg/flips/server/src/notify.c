/*** L4-SPECIFIC INCLUDES ***/
#include <l4/env/errno.h>
#include <l4/util/util.h>
#include <l4/log/l4log.h>
#include <l4/l4vfs/select_listener.h>
#include <l4/l4vfs/types.h>
#include <l4/dde_linux/dde.h>

/*** LOCAL INCLUDES ***/
#include "liblinux.h"
#include "private_socket.h"

#define MAX_SELECT_FDS 128

void notify_select(void);
void notify_select(void)
{
	int fd, ret, n;
	socket_private_t *sp;

	for (;;)
	{
		/* get highest file descriptor that was selected */
		n = liblinux_get_max_fd();

		for (fd = 0; fd < n+1; fd++)
		{
			/* check if fd is non blocking */
			ret = liblinux_select(fd);

			if (ret <= 0)
			{
				continue;
			}
			else
			{
				sp = socket_get_private_data(fd);
				if (sp)
				{
					//LOG("send notification foundfd=%d maxfd=%d", fd, n);
					/* send message to notify thread */
					l4vfs_select_listener_send_notification(
		                            (l4_threadid_t) *(sp->notif_tid),
                		             fd,
		                             ret);
				}
			}
		}
		schedule();
	}
}

/* init and startup of notify thread */
void notify_thread(void)
{
	l4dde_process_add_worker();
	liblinux_init_notify();
	l4thread_started(NULL);
	notify_select();
}
