/*** L4-SPECIFIC INCLUDES ***/
#include <l4/log/l4log.h>
#include <l4/lock/lock.h>
#include <l4/l4vfs/types.h>

#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <net/sock.h>

/*** LOCAL INCLUDES ***/
#include "private_socket.h"
#include "liblinux.h"

/* taken from linux/fs/select.c */
#define POLLIN_SET (POLLRDNORM | POLLRDBAND | POLLIN | POLLHUP | POLLERR)
#define POLLOUT_SET (POLLWRBAND | POLLWRNORM | POLLOUT | POLLERR)
#define POLLEX_SET (POLLPRI)

#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

static struct task_struct *notif_tsk;
static int max_fd;
static l4lock_t lock = L4LOCK_UNLOCKED;

void liblinux_init_notify(void)
{
	notif_tsk = current;
};

/**
 *  add notify thread to wait queue of
 *  socket
 *  */
int liblinux_notify_request(int fd, int mode, l4_threadid_t *notif_tid)
{
	int err;
	struct socket *sock;
	struct sock *sk;
	socket_private_t *sp;

	sock = sockfd_lookup(fd, &err);

	if (! sock) {
		return err;
	}

	sp = (socket_private_t *) socket_get_private_data(fd);
	if (! sp) {
		sp = (socket_private_t *) kmalloc(sizeof(socket_private_t), GFP_KERNEL);
		if (! sp) {
			LOG_Error("not enough memory to allocate socket private");
			return -ENOMEM;
		}
		memset(sp, 0, sizeof(socket_private_t));

		sp->mode = mode;
		sp->notif_tid = (l4_threadid_t *) notif_tid;

		sk = sock->sk;

		LOGd(_DEBUG,"set on waitqueue of sock");

		init_waitqueue_entry(&(sp->q), notif_tsk);
		add_wait_queue(sk->sleep, &(sp->q));
	}
	else {
		sp->mode |= mode;
		sp->notif_tid = (l4_threadid_t *) notif_tid;
	}

	socket_set_private_data(fd, (void *) sp);

	l4lock_lock(&lock);
	if (fd > max_fd) {
		max_fd = fd;
	}
	l4lock_unlock(&lock);

	/* wake up notify thread
	 * to test if socket is already nonblocking */
	wake_up_process(notif_tsk);

	return 0;
}

/**
 * remove notify thread from wait queue of
 * socket
 * */
int liblinux_notify_clear(int fd, int mode, l4_threadid_t *notif_tid)
{
	int err, i, res;
	struct socket *sock;
	struct sock *sk;
	socket_private_t *sp, *p;

	sock = sockfd_lookup(fd, &err);

	if (! sock) {
		return err;
	}

	sp = socket_get_private_data(fd);

	if (! sp) {
		LOGd(_DEBUG,"No private data found for fd (%d)", fd);
		return -EINVAL;
	}

	res = sp->mode - mode;

	/* check if we have to clear the notification */
	if (res <= 0) {
		sk = sock->sk;

		remove_wait_queue(sk->sleep, &(sp->q));
		socket_set_private_data(fd, NULL);
		kfree(sp);

		l4lock_lock(&lock);
		if (fd == max_fd)
		{
			for (i=fd; i > 0; i--) {
				p = socket_get_private_data(i-1);
				if (! p) {
					max_fd--;
				}
			}
		}
		l4lock_unlock(&lock);

	}
	else
	{
		/* check if it contains given mode */
		if (sp->mode & mode) {
			/* remove given mode */
			sp->mode -= mode;
			socket_set_private_data(fd, sp);
	}
	}

	return 0;
}

/**
 * check if sockets are nonblocking
 * at this moment.
 */
int liblinux_select(int fd)
{
	int mask, err, ret = -1;
	struct socket *sock;
	socket_private_t *sp;

	set_current_state(TASK_INTERRUPTIBLE);

	sock = sockfd_lookup(fd, &err);

	if (! sock) {
		LOGd(_DEBUG,"Unknown fd (%d), err (%d)", fd, err);
		return -EINVAL;
	}

	sp = (socket_private_t *) socket_get_private_data(fd);

	if (! sp) {
		LOGd(_DEBUG,"No private data found for fd (%d)", fd);
		return -EINVAL;
	}

	/* poll means to check if socket is nonblocking
	* at this moment */
	mask = sock->ops->poll(NULL, sock, NULL);

	/* check for each nonblock operation */
	if (mask & POLLIN_SET) {
		sp->non_block_mode = SELECT_READ;
	}

	if (mask & POLLOUT_SET) {
		sp->non_block_mode |= SELECT_WRITE;
	}

	if (mask & POLLEX_SET) {
		sp->non_block_mode |= SELECT_EXCEPTION;
	}

	LOGd(_DEBUG,"fd (%d), non_block_mode (%d)", fd,
              sp->non_block_mode);

	ret = sp->mode & sp->non_block_mode;

	return ret;
}

int liblinux_get_max_fd()
{
	int n;

	n = max_fd;
	return n;
}
